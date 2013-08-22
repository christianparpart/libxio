#pragma once

#include <xio/Api.h>
#include <unordered_map>

#include <cstdint>
#include <string>
#include <netinet/in.h> // in_addr, in6_addr

namespace xio {

class XIO_API IPAddress
{
public:
	static const int V4 = AF_INET;
	static const int V6 = AF_INET6;

private:
	int family_;
	uint8_t buf_[sizeof(struct in6_addr)];

public:
	IPAddress();
	explicit IPAddress(const std::string& text, int family = 0);

	IPAddress& operator=(const std::string& value);
	IPAddress& operator=(const IPAddress& value);

	bool set(const std::string& text, int family);
	bool operator!() const { return family_ == 0; }
	operator bool() const { return family_ == V4 || family_ == V6; }

	int family() const;
	const void *data() const;
	size_t size() const;
	std::string str() const;

	friend bool operator==(const IPAddress& a, const IPAddress& b);
	friend bool operator!=(const IPAddress& a, const IPAddress& b);
};

// {{{ inlines
inline int IPAddress::family() const
{
	return family_;
}

inline const void *IPAddress::data() const
{
	return buf_;
}
// }}}

} // namespace xio

namespace std
{
	template <>
	struct hash<xio::IPAddress> :
		public unary_function<xio::IPAddress, size_t>
	{
		size_t operator()(const xio::IPAddress& v) const
		{
			return *(uint32_t*)(v.data());
		}
	};
}
