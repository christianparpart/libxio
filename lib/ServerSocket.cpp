/* <src/ServerSocket.cpp>
 *
 * This file is part of the x0 web server project and is released under GPL-3.
 * http://www.xzero.io/
 *
 * (c) 2009-2013 Christian Parpart <trapni@gmail.com>
 */

#include <xio/ServerSocket.h>
#include <xio/SocketDriver.h>
#include <xio/SocketSpec.h>
#include <xio/Socket.h>
#include <xio/IPAddress.h>
#include <xio/sysconfig.h>

#include <xio/InetServer.h>
#include <xio/UnixServer.h>

#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>

#include "sd-daemon.h"

namespace xio {

#if 0 // !defined(XIO_NDEBUG)
#	define TRACE(msg...) do { printf(msg); printf("\n"); } while (0)
#else
#	define TRACE(msg...) do { } while (0)
#endif

#if !defined(SO_REUSEPORT)
#	define SO_REUSEPORT 15
#endif

// {{{ helpers for finding x0d-inherited file descriptors

// EnvvarFormat ::= [PID ':'] (ListenFD *(';' ListenFD))
// ListenFD     ::= InetFD | UnixFD
// InetFD		::= ADDRESS ':' PORT ':' FD
// UnixFD		::= PATH ':' FD
//
// PID          ::= NUMBER
// NUMBER		::= [0-9]+
// ADDRESS		::= <an IPv4 or IPv6 address>
// PORT			::= NUMBER
// PATH			::= <a UNIX local path>
// FD			::= NUMBER
// 
// the PID part is not yet supported

#define X0_LISTEN_FDS "XZERO_LISTEN_FDS"

static inline int validateSocket(int fd)
{
	struct stat st;
	if (fstat(fd, &st) < 0)
		return -errno;

	if (!S_ISSOCK(st.st_mode))
		return -(errno = ENOTSOCK);

	return 0;
}

/** retrieves the full list of file descriptors passed to us by a parent x0d process.
 */
std::vector<int> ServerSocket::getInheritedSocketList()
{
	std::vector<int> list;

	char* e = getenv(X0_LISTEN_FDS);
	if (!e)
		return list;

	e = strdup(e);

	char* s1 = nullptr;
	while (char* token = strtok_r(e, ";", &s1)) {
		e = nullptr;

		char* s2 = nullptr;

		strtok_r(token, ",", &s2); // IPv4/IPv6 address, if tcp socket, local path otherwise
		char* vival1 = strtok_r(nullptr, ",", &s2); // fd, or tcp port, if tcp socket
		char* vival2 = strtok_r(nullptr, ",", &s2); // fd, if unix domain socket

		if (vival2) {
			list.push_back(atoi(vival2));
		} else {
			list.push_back(atoi(vival1));
		}
	}

	free(e);
	return list;
}

static int getSocketInet(const char* address, int port)
{
	char* e = getenv(X0_LISTEN_FDS);
	if (!e)
		return -1;

	int fd = -1;
	e = strdup(e);

	char* s1 = nullptr;
	while (char* token = strtok_r(e, ";", &s1)) {
		e = nullptr;

		char* s2 = nullptr;

		char* vaddr = strtok_r(token, ",", &s2);
		if (strcmp(vaddr, address) != 0)
			continue;

		char* vport = strtok_r(nullptr, ",", &s2);
		if (atoi(vport) != port)
			continue;

		char* vfd = strtok_r(nullptr, ",", &s2);

		fd = atoi(vfd);
		if (validateSocket(fd) < 0)
			goto err;

		goto done;
	}

err:
	fd = -1;

done:
	free(e);
	return fd;
}

static int getSocketUnix(const char *path)
{
	char* e = getenv(X0_LISTEN_FDS);
	if (!e)
		return -1;

	int fd = -1;
	e = strdup(e);

	char* s1 = nullptr;
	while (char* token = strtok_r(e, ";", &s1)) {
		e = nullptr;

		char* s2 = nullptr;

		char* vaddr = strtok_r(token, ",", &s2);
		if (strcmp(vaddr, path) != 0)
			continue;

		char* vfd = strtok_r(nullptr, ",", &s2);

		fd = atoi(vfd);
		if (validateSocket(fd) < 0)
			goto err;

		goto done;
	}

err:
	fd = -1;

done:
	free(e);
	return fd;
}
// }}}

/*!
 * \addtogroup base
 * \class ServerSocket
 * \brief represents a server listening socket (TCP/IP (v4 and v6) and Unix domain)
 *
 * \see Socket, SocketDriver
 */

/*!
 * \fn ServerSocket::address()
 * \brief retrieves the server-socket's local address it is listening to.
 *
 * This is either an IPv4, IPv6-address or a local path to the UNIX domain socket.
 *
 * \see ServerSocket::port()
 */

/*!
 * \fn ServerSocket::port()
 * \brief retrieves the server-socket's local port it is listening to (or 0 on UNIX domain sockets).
 * \see ServerSocket::address()
 */

/*! initializes a server socket.
 * \param loop a pointer to the event-loop handler to serve this socket.
 */
ServerSocket::ServerSocket(struct ev_loop* loop) :
	loop_(loop),
	errorText_(),
	socketDriver_(new SocketDriver()),
	fd_(-1),
	flags_(0),
	typeMask_(0),
	backlog_(SOMAXCONN),
	multiAcceptCount_(1),
	io_(loop),
	callback_(nullptr),
	callbackData_(nullptr)
{
	io_.set<ServerSocket, &ServerSocket::accept>(this);
}

/*! safely destructs the server socket.
 *
 * This closes the file descriptor if opened and also \e deletes the internally used socket driver.
 */
ServerSocket::~ServerSocket()
{
	if (isActive())
		stop();

	if (isOpen())
		close();

	setSocketDriver(nullptr);
}

/*! sets the backlog for the listener socket.
 * \note must NOT be called if socket is already open.
 */
void ServerSocket::setBacklog(int value)
{
	assert(isOpen() == false);
	backlog_ = value;
}

ServerSocket* ServerSocket::open(const SocketSpec& spec, int flags)
{
	// TODO
#if 1
	return nullptr;
#else
	if (spec.backlog() > 0)
		setBacklog(spec.backlog());

	reusePort_ = spec.reusePort();

	if (spec.isLocal())
		return open(spec.local(), flags);
	else
		return open(spec.ipaddr().str(), spec.port(), flags);
#endif
}

void ServerSocket::start()
{
	TRACE("start()");

	assert(isOpen() && "Cannot start to watch on a server socket that's not set up.");
	assert(!isActive() && "Server socket already actively watching.");

	io_.set(fd_, ev::READ);
	io_.start();
}

void ServerSocket::stop()
{
	TRACE("stop()");

	if (!isActive())
		return;

	io_.stop();
}

/*! stops listening and closes the server socket.
 *
 * \see open()
 */
void ServerSocket::close()
{
	if (fd_ < 0)
		return;

	stop();

	::close(fd_);
	fd_ = -1;
}

void ServerSocket::setMultiAcceptCount(size_t value)
{
	multiAcceptCount_ = std::max(value, static_cast<size_t>(1));
}

/*! defines a socket driver to be used for creating the client sockets.
 *
 * This is helpful when you want to create an SSL-aware server-socket, then set a custom socket driver, that is
 * creating subclassed SSL-aware Socket instances.
 *
 * See the ssl plugin as an example.
 */
void ServerSocket::setSocketDriver(SocketDriver* sd)
{
	if (socketDriver_ == sd)
		return;

	if (socketDriver_)
		delete socketDriver_;

	socketDriver_ = sd;
}

void ServerSocket::accept(ev::io&, int)
{
#if defined(WITH_MULTI_ACCEPT)
	for (size_t n = multiAcceptCount_; n > 0; --n) {
		if (!acceptOne()) {
			break;
		}
	}
#else
	acceptOne();
#endif
}

inline bool ServerSocket::acceptOne()
{
#if defined(HAVE_ACCEPT4) && defined(WITH_ACCEPT4)
	bool flagged = true;
	int cfd = ::accept4(fd_, nullptr, 0, typeMask_);
	if (cfd < 0 && errno == ENOSYS) {
		cfd = ::accept(fd_, nullptr, 0);
		flagged = false;
	}
#else
	bool flagged = false;
	int cfd = ::accept(fd_, nullptr, 0);
#endif

	if (cfd < 0) {
		switch (errno) {
		case EINTR:
		case EAGAIN:
#if EAGAIN != EWOULDBLOCK
		case EWOULDBLOCK:
#endif
			goto out;
		default:
			goto err;
		}
	}

	if (!flagged && flags_ && fcntl(cfd, F_SETFL, fcntl(cfd, F_GETFL) | flags_) < 0)
		goto err;

	TRACE("accept(): %d", cfd);

	callback(createSocket(cfd), this);

	return true;

err:
	// notify callback about the error on accept()
	::close(cfd);
	callback(nullptr, this);

out:
	// abort the outer loop
	return false;
}

/** enables/disables CLOEXEC-flag on the server listener socket.
 *
 * \note this does not affect future client socket flags.
 */
bool ServerSocket::setCloseOnExec(bool enable)
{
	unsigned flags = enable
		? fcntl(fd_, F_GETFD) | FD_CLOEXEC
		: fcntl(fd_, F_GETFD) & ~FD_CLOEXEC;

	if (fcntl(fd_, F_SETFD, flags) < 0)
		return false;

	return true;
}

bool ServerSocket::isCloseOnExec() const
{
	return fcntl(fd_, F_GETFD) & FD_CLOEXEC;
}

bool ServerSocket::setNonBlocking(bool enable)
{
	unsigned flags = enable
		? fcntl(fd_, F_GETFL) | O_NONBLOCK 
		: fcntl(fd_, F_GETFL) & ~O_NONBLOCK;

	if (fcntl(fd_, F_SETFL, flags) < 0)
		return false;

	return true;
}

bool ServerSocket::isNonBlocking() const
{
	return fcntl(fd_, F_GETFL) & O_NONBLOCK;
}

int ServerSocket::setOption(int level, int option, int value)
{
	socklen_t val = value;
	return ::setsockopt(fd_, level, option, &val, sizeof(val));
}

bool ServerSocket::listen(int backlog)
{
	if (backlog < 0)
		backlog = backlog_;

	if (::listen(fd_, backlog) < 0)
		return false;

	backlog_ = backlog;
	return true;
}

// {{{ InetServer
InetServer::InetServer(struct ev_loop* loop) :
	ServerSocket(loop),
	ipaddr_(),
	port_(-1),
	reusePort_(false)
{
}

InetServer::~InetServer()
{
}

void InetServer::setReusePort(bool value)
{
	assert(isOpen() == false);
	reusePort_ = value;
}

InetServer* InetServer::clone(struct ev_loop* loop) const
{
	auto s = new InetServer(loop);

	s->setBacklog(backlog_);
	s->setReusePort(reusePort_);
	s->open(ipaddr_, port_, flags_);

	return s;
}

/*! starts listening on a TCP/IP (v4 or v6) address.
 *
 * \param ipaddr the IP address to bind to
 * \param port the TCP/IP port number to listen to
 * \param flags some flags, such as O_CLOEXEC and O_NONBLOCK (the most prominent) to set on server socket and each created client socket
 *
 * \retval true successfully initialized
 * \retval false some failure occured during setting up the server socket.
 */
bool InetServer::open(const IPAddress& ipaddr, int port, int flags)
{
	int sd_fd_count = sd_listen_fds(false);

	typeMask_ = 0;
	flags_ = flags;

	if (flags & O_CLOEXEC) {
		flags_ &= ~O_CLOEXEC;
		typeMask_ |= SOCK_CLOEXEC;
	}

	if (flags & O_NONBLOCK) {
		flags_ &= ~O_NONBLOCK;
		typeMask_ |= SOCK_NONBLOCK;
	}

	// check if passed by parent x0d first
	if ((fd_ = getSocketInet(ipaddr.str().c_str(), port)) >= 0) {
		// socket found, but ensure our expected `flags` are set.
		if ((flags & O_NONBLOCK) && fcntl(fd_, F_SETFL, fcntl(fd_, F_GETFL) | O_NONBLOCK) < 0)
			goto syserr;

		if ((flags & O_CLOEXEC) && fcntl(fd_, F_SETFD, fcntl(fd_, F_GETFD) | FD_CLOEXEC) < 0)
			goto syserr;

		goto done;
	}

	// check if systemd created the socket for us
	if (sd_fd_count > 0) {
		fd_ = SD_LISTEN_FDS_START;
		int last = fd_ + sd_fd_count;

		for (; fd_ < last; ++fd_) {
			if (sd_is_socket_inet(fd_, ipaddr.family(), SOCK_STREAM, true, port) > 0) {
				// socket found, but ensure our expected `flags` are set.
				if ((flags & O_NONBLOCK) && fcntl(fd_, F_SETFL, fcntl(fd_, F_GETFL) | O_NONBLOCK) < 0)
					goto syserr;

				if ((flags & O_CLOEXEC) && fcntl(fd_, F_SETFD, fcntl(fd_, F_GETFD) | FD_CLOEXEC) < 0)
					goto syserr;

#if defined(TCP_QUICKACK)
				if (setOption(SOL_TCP, TCP_QUICKACK, 1) < 0)
					goto syserr;
#endif

#if defined(TCP_DEFER_ACCEPT) && defined(WITH_TCP_DEFER_ACCEPT)
				if (setOption(SOL_TCP, TCP_DEFER_ACCEPT, 1) < 0)
					goto syserr;
#endif

				goto done;
			}
		}

		char buf[256];
		snprintf(buf, sizeof(buf), "Running under systemd socket unit, but we received no socket for %s port %d.", ipaddr.str().c_str(), port);
		errorText_ = buf;
		goto err;
	}

	// create socket manually
	fd_ = socket(ipaddr.family(), SOCK_STREAM | typeMask_, IPPROTO_TCP);
	if (fd_ < 0)
		goto syserr;

	// force non-blocking
	if ((flags & O_NONBLOCK) && fcntl(fd_, F_SETFL, fcntl(fd_, F_GETFL) | O_NONBLOCK) < 0)
		goto syserr;

	// force close-on-exec
	if ((flags & O_CLOEXEC) && fcntl(fd_, F_SETFD, fcntl(fd_, F_GETFD) | FD_CLOEXEC) < 0)
		goto syserr;

	// force address reuse
#if defined(SO_REUSEADDR)
	if (setOption(SOL_SOCKET, SO_REUSEADDR, 1) < 0)
		goto syserr;
#endif

#if defined(SO_REUSEPORT)
	if (reusePort_) {
		reusePort_ = setOption(SOL_SOCKET, SO_REUSEPORT, 1) == 0;
		// enabling this can still fail if the OS doesn't support it (ENOPROTOOPT)
	}
#endif

#if defined(TCP_QUICKACK)
	if (setOption(SOL_TCP, TCP_QUICKACK, 1) < 0)
		goto syserr;
#endif

#if defined(TCP_DEFER_ACCEPT) && defined(WITH_TCP_DEFER_ACCEPT)
	if (setOption(SOL_TCP, TCP_DEFER_ACCEPT, 1) < 0)
		goto syserr;
#endif

	// TODO so_linger(false, 0)
	// TODO so_keepalive(true)

	if (!bind(ipaddr, port))
		goto syserr;

	if (!listen(backlog_))
		goto syserr;

	goto done;

done:
	start();

	return true;

syserr:
	errorText_ = strerror(errno);

err:
	if (fd_ >= 0) {
		::close(fd_);
		fd_ = -1;
	}

	return false;
}

bool InetServer::bind(const IPAddress& ipaddr, int port)
{
	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	memcpy(&sin.sin_addr, ipaddr.data(), ipaddr.size());
	sin.sin_port = htons(port);

	if (::bind(fd_, (const sockaddr*) &sin, sizeof(sin)) < 0)
		return false;

	ipaddr_ = ipaddr;
	port_ = port;
	return true;
}

void InetServer::close()
{
	ServerSocket::close();
	reusePort_ = false;
}

Socket* InetServer::createSocket(int cfd)
{
	return socketDriver_->create(loop_, cfd, ipaddr_.family());
}

std::string InetServer::serialize() const
{
	char buf[1024];
	auto n = snprintf(buf, sizeof(buf), "%s,%d,%d", ipaddr_.str().c_str(), port_, fd_);
	return std::string(buf, n);
}
// }}}
// {{{ UnixServer
UnixServer::UnixServer(struct ev_loop* loop) :
	ServerSocket(loop),
	path_()
{
}

UnixServer::~UnixServer()
{
}

UnixServer* UnixServer::clone(struct ev_loop* loop) const
{
	auto s = new UnixServer(loop);

	s->open(path_, flags_);

	return s;
}

/*! starts listening on a UNIX domain server socket.
 *
 * \param path the path on the local file system, that this socket is to listen to.
 * \param flags some flags, such as O_CLOEXEC and O_NONBLOCK (the most prominent) to set on server socket and each created client socket
 *
 * \retval true successfully initialized
 * \retval false some failure occured during setting up the server socket.
 */
bool UnixServer::open(const std::string& path, int flags)
{
	int sd_fd_count = sd_listen_fds(false);

	typeMask_ = 0;
	flags_ = flags;

	if (flags & O_CLOEXEC) {
		flags_ &= ~O_CLOEXEC;
		typeMask_ |= SOCK_CLOEXEC;
	}

	if (flags & O_NONBLOCK) {
		flags_ &= ~O_NONBLOCK;
		typeMask_ |= SOCK_NONBLOCK;
	}

	// check if passed by parent process first
	if ((fd_ = getSocketUnix(path.c_str())) >= 0) {
		// socket found, but ensure our expected `flags` are set.
		if (flags && fcntl(fd_, F_SETFL, fcntl(fd_, F_GETFL) | flags) < 0) {
			goto syserr;
		} else {
			goto done;
		}
	}

	// check if systemd created the socket for us
	if (sd_fd_count > 0) {
		fd_ = SD_LISTEN_FDS_START;
		int last = fd_ + sd_fd_count;

		for (; fd_ < last; ++fd_) {
			if (sd_is_socket_unix(fd_, AF_UNIX, SOCK_STREAM, path.c_str(), path.size()) > 0) {
				// socket found, but ensure our expected `flags` are set.
				if (flags && fcntl(fd_, F_SETFL, fcntl(fd_, F_GETFL) | flags) < 0) {
					goto syserr;
				} else {
					goto done;
				}
			}
		}

		errorText_ = "Running under systemd socket unit, but we received no UNIX-socket for \"" + path + "\".";
		goto err;
	}

	// create socket manually
	fd_ = ::socket(PF_UNIX, SOCK_STREAM, 0);
	if (fd_ < 0)
		goto syserr;

	if (flags && fcntl(fd_, F_SETFL, fcntl(fd_, F_GETFL) | flags) < 0)
		goto syserr;

	if (!bind(path))
		goto syserr;

	if (!listen(backlog_))
		goto syserr;

	if (chmod(path.c_str(), 0666) < 0) {
		perror("chmod");
	}

done:
	start();

	return true;

syserr:
	errorText_ = strerror(errno);

err:
	if (fd_ >= 0) {
		::close(fd_);
		fd_ = -1;
	}

	return false;
}

bool UnixServer::bind(const std::string& path)
{
	struct sockaddr_un addr;
	addr.sun_family = AF_UNIX;

	if (path.size() >= sizeof(addr.sun_path)) {
		errno = ENAMETOOLONG;
		return false;
	}

	size_t addrlen;
	addrlen = sizeof(addr.sun_family)
		+ strlen(strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path)));

	if (::bind(fd_, reinterpret_cast<struct sockaddr*>(&addr), addrlen) < 0)
		return false;

	path_ = path;

	return true;
}

void UnixServer::close()
{
	ServerSocket::close();

	if (!path_.empty()) {
		::unlink(path_.c_str());
	}
}

Socket* UnixServer::createSocket(int cfd)
{
	return socketDriver_->create(loop_, cfd, AF_UNIX);
}

std::string UnixServer::serialize() const
{
	char buf[4096];
	auto n = snprintf(buf, sizeof(buf), "%s,%d",    path_.c_str(), fd_);
	return std::string(buf, n);
}

// }}}

} // namespace xio
