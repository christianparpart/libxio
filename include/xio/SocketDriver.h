#pragma once

#include <xio/Api.h>
#include <system_error>
#include <unistd.h>
#include <ev++.h>

namespace xio {

class Socket;
class IPAddress;

class XIO_API SocketDriver
{
public:
	SocketDriver();
	virtual ~SocketDriver();

	virtual bool isSecure() const;
	virtual Socket *create(struct ev_loop *loop, int handle, int af);
	virtual Socket *create(struct ev_loop *loop, IPAddress* ipaddr, int port);
	virtual void destroy(Socket *);
};

} // namespace xio
