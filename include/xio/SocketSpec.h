#pragma once
/* <SocketSpec.h>
 *
 * This file is part of the x0 web server project and is released under GPL-3.
 * http://www.xzero.io/
 *
 * (c) 2009-2013 Christian Parpart <trapni@gmail.com>
 */

#include <xio/Api.h>
#include <xio/IPAddress.h>
#include <string>

namespace xio {

class XIO_API SocketSpec
{
public:
	enum Type {
		Unknown,
		Local,
		Inet,
	};

	SocketSpec();
	SocketSpec(const SocketSpec& ss);
	SocketSpec(const IPAddress& ipaddr, int port, int backlog = -1, size_t maccept = 1, bool reusePort = false) :
		type_(Inet),
		ipaddr_(ipaddr),
		port_(port),
		backlog_(backlog),
		multiAcceptCount_(maccept),
		reusePort_(reusePort)
	{}

	static SocketSpec fromString(const std::string& value);
	static SocketSpec fromLocal(const std::string& path, int backlog = -1);
	static SocketSpec fromInet(const IPAddress& ipaddr, int port, int backlog = -1);

	void clear();

	Type type() const { return type_; }
	bool isValid() const { return type_ != Unknown; }
	bool isLocal() const { return type_ == Local; }
	bool isInet() const { return type_ == Inet; }

	const IPAddress& ipaddr() const { return ipaddr_; }
	int port() const { return port_; }
	const std::string& local() const { return local_; }
	int backlog() const { return backlog_; }
	size_t multiAcceptCount() const { return multiAcceptCount_; }
	bool reusePort() const { return reusePort_; }

	void setPort(int value);
	void setBacklog(int value);
	void setMultiAcceptCount(size_t value);
	void setReusePort(bool value);

	std::string str() const;

private:
	Type type_;
	IPAddress ipaddr_;
	std::string local_;
	int port_;
	int backlog_;
	size_t multiAcceptCount_;
	bool reusePort_;
};


XIO_API bool operator==(const xio::SocketSpec& a, const xio::SocketSpec& b);
XIO_API bool operator!=(const xio::SocketSpec& a, const xio::SocketSpec& b);

inline XIO_API bool operator==(const xio::SocketSpec& a, const xio::SocketSpec& b)
{
	if (a.type() != b.type())
		return false;

	switch (a.type()) {
		case xio::SocketSpec::Local:
			return a.local() == b.local();
		case xio::SocketSpec::Inet:
			return a.port() == b.port() && a.ipaddr() == b.ipaddr();
		default:
			return false;
	}
}

inline XIO_API bool operator!=(const xio::SocketSpec& a, const xio::SocketSpec& b)
{
	return !(a == b);
}
} // namespace x0

namespace std
{
	template <>
	struct hash<xio::SocketSpec> :
		public unary_function<xio::SocketSpec, size_t>
	{
		size_t operator()(const xio::SocketSpec& v) const
		{
			switch (v.type()) {
				case xio::SocketSpec::Inet:
					return hash<size_t>()(v.type()) ^ hash<xio::IPAddress>()(v.ipaddr());
				case xio::SocketSpec::Local:
					return hash<size_t>()(v.type()) ^ hash<string>()(v.local());
				default:
					return 0;
			}
		}
	};
}
