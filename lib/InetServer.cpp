/* <src/ServerSocket.cpp>
 *
 * This file is part of the x0 web server project and is released under GPL-3.
 * http://www.xzero.io/
 *
 * (c) 2009-2013 Christian Parpart <trapni@gmail.com>
 */

#include <xio/InetServer.h>
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

#define X0_LISTEN_FDS "XZERO_LISTEN_FDS"

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

		struct stat st;
		if (fstat(fd, &st) < 0)
			goto err;

		if (!S_ISSOCK(st.st_mode)) {
			errno = ENOTSOCK;
			goto err;
		}

		goto done;
	}

err:
	fd = -1;

done:
	free(e);
	return fd;
}

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

	if (!bind(ipaddr, port)) {
		perror("bind");
		goto syserr;
	}

	if (!listen(backlog_)) {
		perror("listen");
		goto syserr;
	}

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

} // namespace xio
