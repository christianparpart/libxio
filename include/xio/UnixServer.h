#pragma once

#include <xio/Api.h>
#include <xio/ServerSocket.h>
#include <string>
#include <vector>
#include <ev++.h>

namespace xio {

class XIO_API UnixServer : public ServerSocket
{
public:
	UnixServer(struct ev_loop* loop);
	~UnixServer();

	bool open(const std::string& localAddress, int flags);
	bool bind(const std::string& path);
	virtual void close();

	virtual std::string serialize() const;
	virtual UnixServer* clone(struct ev_loop* loop) const;

protected:
	virtual Socket* createSocket(int cfd);

private:
	std::string path_;
};

} // namespace xio
