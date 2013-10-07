#pragma once

#include <xio/Api.h>
#include <xio/ServerSocket.h>
#include <xio/IPAddress.h>

namespace xio {

class XIO_API InetServer : public ServerSocket
{
public:
	explicit InetServer(struct ev_loop* loop);
	~InetServer();

	bool open(const IPAddress& ipAddress, int port, int flags = 0);

	bool bind(const IPAddress& ipaddr, int port);

	void setReusePort(bool enabled);
	bool reusePort() const { return reusePort_; }

	const IPAddress& ipaddr() const { return ipaddr_; }
	int port() const { return port_; }

	virtual std::string serialize() const;
	virtual InetServer* clone(struct ev_loop* loop) const;
	virtual void close();

protected:
	virtual Socket* createSocket(int cfd);

private:
	IPAddress ipaddr_;
	int port_;

	bool reusePort_;
};

} // namespace xio
