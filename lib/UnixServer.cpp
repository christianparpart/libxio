/* <src/ServerSocket.cpp>
 *
 * This file is part of the x0 web server project and is released under GPL-3.
 * http://www.xzero.io/
 *
 * (c) 2009-2013 Christian Parpart <trapni@gmail.com>
 */

#include <xio/UnixServer.h>
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

static inline int getSocketUnix(const char *path)
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

} // namespace xio
