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

} // namespace xio
