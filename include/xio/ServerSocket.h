#pragma once
/* <src/ServerSocket.h>
 *
 * This file is part of the x0 web server project and is released under GPL-3.
 * http://www.xzero.io/
 *
 * (c) 2009-2013 Christian Parpart <trapni@gmail.com>
 */

#include <xio/Api.h>
#include <string>
#include <vector>
#include <functional>
#include <ev++.h>

namespace xio {

class Socket;
class SocketSpec;
class SocketDriver;

class XIO_API ServerSocket
{
public:
	explicit ServerSocket(struct ev_loop* loop);
	virtual ~ServerSocket();

	static ServerSocket* open(const SocketSpec& spec, int flags);

	bool listen(int backlog = -1);

	void setSocketDriver(SocketDriver* sd);
	SocketDriver* socketDriver() { return socketDriver_; }
	const SocketDriver* socketDriver() const { return socketDriver_; }

	bool isCloseOnExec() const;
	bool setCloseOnExec(bool enable);

	bool isNonBlocking() const;
	bool setNonBlocking(bool enable);

	void setBacklog(int value);
	int backlog() const { return backlog_; }

	size_t multiAcceptCount() const { return multiAcceptCount_; }
	void setMultiAcceptCount(size_t value);

	template<typename K, void (K::*cb)(Socket*, ServerSocket*)>
	void set(K* object);

	const std::string& errorText() const { return errorText_; }

	virtual std::string serialize() const = 0;
	static std::vector<int> getInheritedSocketList();

	virtual ServerSocket* clone(struct ev_loop* loop) const = 0;

	int handle() const { return fd_; }
	bool isOpen() const { return fd_ >= 0; }
	bool isActive() const { return io_.is_active(); }

	virtual void start();
	virtual void stop();
	virtual void close();

	std::function<void(Socket*, ServerSocket*)> callback;

private:
	template<typename K, void (K::*cb)(Socket*, ServerSocket*)>
	static void callback_thunk(Socket* cs, ServerSocket* ss);

	void accept(ev::io&, int);

protected:
	virtual bool acceptOne();
	virtual Socket* createSocket(int cfd) = 0;

	int setOption(int level, int option, int value);

protected:
	struct ev_loop* loop_;
	std::string errorText_;
	SocketDriver* socketDriver_;
	int fd_;
	int flags_;
	int typeMask_;
	int backlog_;
	size_t multiAcceptCount_;
	ev::io io_;

	void (*callback_)(Socket*, ServerSocket*);
	void* callbackData_;
};

// {{{
template<typename K, void (K::*cb)(Socket*, ServerSocket*)>
void ServerSocket::set(K* object)
{
	callback_ = &callback_thunk<K, cb>;
	callbackData_ = object;
}

template<typename K, void (K::*cb)(Socket*, ServerSocket*)>
void ServerSocket::callback_thunk(Socket* cs, ServerSocket* ss)
{
	(static_cast<K*>(ss->callbackData_)->*cb)(cs, ss);
}
// }}}

} // namespace xio
