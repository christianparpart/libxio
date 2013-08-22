#include <xio/Socket.h>
#include <xio/IPAddress.h>
#include <xio/KernelBuffer.h>
#include <xio/StreamVisitor.h>
#include <sys/sendfile.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <assert.h>

namespace xio {

#if !defined(XZERO_NDEBUG)
#	define TRACE(msg...) do { printf(msg); printf("\n"); } while (0)
#else
#	define TRACE(msg...) do { } while (0)
#endif

Socket::Socket(struct ev_loop* loop) :
	fd_(-1),
	state_(Closed),
	io_(loop),
	timer_(loop),
	handler_()
{
	initialize();
}

Socket::Socket(struct ev_loop* loop, int fd, int af, State state) :
	fd_(fd),
	state_(state),
	io_(loop),
	timer_(loop),
	handler_()
{
	(void) af;

	initialize();
}

Socket::~Socket()
{
	close();
}

void Socket::initialize()
{
	io_.set<Socket, &Socket::io>(this);
	timer_.set<Socket, &Socket::timeout>(this);
}

void Socket::close()
{
	if (fd_ >= 0) {
		::close(fd_);
		state_ = Closed;
	}
}

bool Socket::open(const SocketSpec& spec, int flags)
{
	return false; // TODO
}

static inline bool applyFlags(int fd, int flags)
{
	return flags
		? fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | flags) == 0
		: true;
}

Socket::State Socket::open(const IPAddress& ipaddr, int port, int flags)
{
	// compute type-mask
	int typeMask = 0;
#if defined(SOCK_NONBLOCK)
	if (flags & O_NONBLOCK) {
		flags &= ~O_NONBLOCK;
		typeMask |= SOCK_NONBLOCK;
	}
#endif

#if defined(SOCK_CLOEXEC)
	if (flags & O_CLOEXEC) {
		flags &= ~O_CLOEXEC;
		typeMask |= SOCK_CLOEXEC;
	}
#endif

	fd_ = ::socket(ipaddr.family(), SOCK_STREAM | typeMask, IPPROTO_TCP);
	if (fd_ < 0)
		return Closed;

	if (!applyFlags(fd_, flags))
		return Closed;

	char buf[sizeof(sockaddr_in6)];
	std::size_t size;
	memset(&buf, 0, sizeof(buf));
	switch (ipaddr.family()) {
		case IPAddress::V4:
			size = sizeof(sockaddr_in);
			((sockaddr_in *)buf)->sin_port = htons(port);
			((sockaddr_in *)buf)->sin_family = AF_INET;
			memcpy(&((sockaddr_in *)buf)->sin_addr, ipaddr.data(), ipaddr.size());
			break;
		case IPAddress::V6:
			size = sizeof(sockaddr_in6);
			((sockaddr_in6 *)buf)->sin6_port = htons(port);
			((sockaddr_in6 *)buf)->sin6_family = AF_INET6;
			memcpy(&((sockaddr_in6 *)buf)->sin6_addr, ipaddr.data(), ipaddr.size());
			break;
		default:
			::close(fd_);
			fd_ = -1;
			return Closed;
	}

	int rv = ::connect(fd_, (struct sockaddr*)buf, size);
	if (rv == 0) {
		state_ = Operational;
	} else if (errno == EINPROGRESS) {
		state_ = Connecting;
	} else {
		::close(fd_);
		fd_ = -1;
		state_ = Closed;
	}

	return state_;
}

Socket* Socket::open(struct ev_loop* loop, const SocketSpec& spec, int flags)
{
	return nullptr; // TODO
}

bool Socket::tcpCork() const
{
	return false;
}

void Socket::setTcpCork(bool enable)
{
}

TimeSpan Socket::lingering() const
{
	return TimeSpan::Zero;
}

void Socket::setLingering(TimeSpan timeout)
{
}

void Socket::on(int mode, TimeSpan timeout, std::function<void(int)> cb)
{
	assert((mode & TIMEOUT) == 0);

	if (mode & CONNECTED) {
		mode &= ~CONNECTED;
		mode |= WRITE;
	}

	handler_ = cb;
	timer_.start(timeout.value(), 0);
	io_.start(fd_, mode);
}

void Socket::start(int mode, TimeSpan timeout)
{
	timer_.start(timeout.value(), 0);
	io_.start(fd_, mode);
}

void Socket::restart()
{
	if (!io_.is_active())
		return;

	if (timer_.is_active())
		timer_.stop();

	timer_.start();
}

void Socket::stop()
{
	timer_.stop();
	io_.stop();
	std::move(handler_);
}

void Socket::io(ev::io&, int revents)
{
	timer_.stop();

	if (state_ == Connecting)
		onConnectComplete();
	else if (state_ == Handshake)
		handler_(HANDSHAKE);
	else if (handler_) {
		handler_(revents);
	}
}

void Socket::onConnectComplete()
{
	stop();

	int val = 0;
	socklen_t vlen = sizeof(val);
	if (getsockopt(fd_, SOL_SOCKET, SO_ERROR, &val, &vlen) == 0) {
		if (val == 0) {
			TRACE("onConnectComplete: connected");
			state_ = Operational;
		} else {
			TRACE("onConnectComplete: error(%d): %s", val, strerror(val));
			close();
			errno = val;
		}
	} else {
		val = errno;
		TRACE("onConnectComplete: getsocketopt() error: %s", strerror(errno));
		close();
		errno = val;
	}

	callback(CONNECTED);
}

void Socket::callback(int mode)
{
	if (handler_) {
		handler_(mode);
	} else {
#if 1 == 0
		if (mode & TIMEOUT)
			on_timeout();

		if (mode & CONNECTED)
			on_connected();

		if (mode & READ)
			on_read();

		if (mode & WRITE)
			on_write();
#endif
	}
}

void Socket::timeout(ev::timer&, int)
{
	timer_.stop();
	io_.stop();

	handler_(Socket::TIMEOUT);
}

size_t Socket::size() const
{
	return 0; // not supported
}

ssize_t Socket::read(void* buf, size_t size)
{
	return ::read(fd_, buf, size);
}

ssize_t Socket::read(Socket* socket, size_t size)
{
	return 0;
}

ssize_t Socket::read(KernelBuffer* pipe, size_t size)
{
	return 0;
}

ssize_t Socket::read(int fd, size_t size)
{
	return 0;
}

ssize_t Socket::read(int fd, off_t *fd_off, size_t size)
{
	return 0;
}

int Socket::read()
{
	char ch = -1;
	if (::read(fd_, &ch, sizeof(ch)) < 0)
		return -1;

	return ch;
}

ssize_t Socket::write(const void* buf, size_t size)
{
	return ::write(fd_, buf, size);
}

ssize_t Socket::write(Socket* socket, size_t size, Mode mode)
{
	return 0;
}

ssize_t Socket::write(KernelBuffer* pipe, size_t size, Mode mode)
{
	return splice(
		pipe->readFd(), nullptr,
		fd_, nullptr,
		size, SPLICE_F_MOVE | SPLICE_F_NONBLOCK | SPLICE_F_MORE
	);
}

ssize_t Socket::write(int fd, off_t *fd_off, size_t size)
{
	return 0;
}

ssize_t Socket::write(int fd, size_t size)
{
	return sendfile(fd_, fd, nullptr, size);
}

void Socket::accept(StreamVisitor& visitor)
{
	visitor.visit(*this);
}

} // namespace xio
