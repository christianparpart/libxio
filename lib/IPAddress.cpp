#include <xio/IPAddress.h>
#include <unordered_map>

#include <cstdint>
#include <string>
#include <string.h>     // memset()
#include <netinet/in.h> // in_addr, in6_addr
#include <arpa/inet.h>  // ntohl(), htonl()
#include <stdio.h>
#include <stdlib.h>

namespace xio {

IPAddress::IPAddress()
{
	family_ = 0;
	memset(buf_, 0, sizeof(buf_));
}

// I suggest to use a very strict IP filter to prevent spoofing or injection
IPAddress::IPAddress(const std::string& text, int family)
{
	if (family != 0) {
		set(text, family);
	// You should use regex to parse ipv6 :) ( http://home.deds.nl/~aeron/regex/ )
	} else if (text.find(':') != std::string::npos) {
		set(text, AF_INET6);
	} else {
		set(text, AF_INET);
	}
}

IPAddress& IPAddress::operator=(const std::string& text)
{
	// You should use regex to parse ipv6 :) ( http://home.deds.nl/~aeron/regex/ )
	if (text.find(':') != std::string::npos) {
		set(text, AF_INET6);
	} else {
		set(text, AF_INET);
	}
	return *this;
}

IPAddress& IPAddress::operator=(const IPAddress& v)
{
	family_ = v.family_;
	memcpy(buf_, v.buf_, v.size());

	return *this;
}

bool IPAddress::set(const std::string& text, int family)
{
	family_ = family;
	int rv = inet_pton(family, text.c_str(), buf_);
	if (rv <= 0) {
		if (rv < 0)
			perror("inet_pton");
		else
			fprintf(stderr, "IP address Not in presentation format: %s\n", text.c_str());

		return false;
	}
	return true;
}

size_t IPAddress::size() const
{
	return family_ == V4
		? sizeof(in_addr)
		: sizeof(in6_addr);
}

std::string IPAddress::str() const
{
	char result[INET6_ADDRSTRLEN];
	inet_ntop(family_, &buf_, result, sizeof(result));

	return result;
}

bool operator==(const IPAddress& a, const IPAddress& b)
{
	if (&a == &b)
		return true;

	if (a.family() != b.family())
		return false;

	switch (a.family()) {
		case AF_INET:
		case AF_INET6:
			return memcmp(a.data(), b.data(), a.size()) == 0;
		default:
			return false;
	}

	return false;
}

bool operator!=(const IPAddress& a, const IPAddress& b)
{
	return !(a == b);
}

} // namespace xio
