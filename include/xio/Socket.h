#pragma once

#include <xio/Stream.h>
#include <xio/TimeSpan.h>
#include <xio/DateTime.h>
#include <functional>
#include <unistd.h>
#include <ev++.h>

namespace xio {

class SocketSpec;
class IPAddress;

class XIO_API Socket : public Stream
{
public:
	enum State {
		Closed,
		Connecting,
		Handshake,
		Operational
	};

	enum Event {
		ERROR = EV_ERROR,
		READ = EV_READ,
		WRITE = EV_WRITE,
		TIMEOUT = 0x100,
		HANDSHAKE = 0x200,
		CONNECTED = 0x400,
	};

	Socket(struct ev_loop* loop);
	Socket(struct ev_loop* loop, int fd, int af, State state = Operational);
	virtual ~Socket();

	void close();
	State open(const IPAddress& ip, int port, int flags = 0);
	bool open(const SocketSpec& spec, int flags = 0);
	static Socket* open(struct ev_loop* loop, const SocketSpec& spec, int flags = 0);

	bool tcpCork() const;
	void setTcpCork(bool enable);

	TimeSpan lingering() const;
	void setLingering(TimeSpan timeout);

	void on(int mode, TimeSpan timeout, std::function<void(int)> cb);
	void start(int mode, TimeSpan timeout);
	void restart();
	void stop();

	State state() const { return state_; }

	// {{{ stream impl
	virtual size_t size() const;

	virtual ssize_t read(void* buf, size_t size);
	virtual ssize_t read(Socket* socket, size_t size);
	virtual ssize_t read(Pipe* pipe, size_t size);
	virtual ssize_t read(int fd, size_t size);
	virtual ssize_t read(int fd, off_t *fd_off, size_t size);
	virtual int read();

	using Stream::write;
	virtual ssize_t write(const void* buf, size_t size);
	virtual ssize_t write(Socket* socket, size_t size, Mode mode = Stream::MOVE);
	virtual ssize_t write(Pipe* pipe, size_t size, Mode mode = Stream::MOVE);
	virtual ssize_t write(int fd, off_t *fd_off, size_t size);
	virtual ssize_t write(int fd, size_t size);

	virtual void accept(StreamVisitor&);
	// }}}

private:
	void initialize();
	void onConnectComplete();
	void io(ev::io&, int);
	void timeout(ev::timer&, int);
	void callback(int mode);

private:
	int fd_;
	State state_;
	ev::io io_;
	ev::timer timer_;
	std::function<void(int)> handler_;
};

} // namespace xio
