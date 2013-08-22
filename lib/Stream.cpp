#include <xio/Stream.h>
#include <string.h>

namespace xio {

bool Stream::empty() const
{
	return size() == 0;
}

ssize_t Stream::write(const char* str)
{
	return write(str, strlen(str));
}

} // namespace xio
