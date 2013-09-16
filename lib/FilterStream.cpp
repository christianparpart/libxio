#include <xio/FilterStream.h>
#include <xio/Filter.h>
#include <xio/Buffer.h>
#include <deque>

namespace xio {

ssize_t FilterStream::process(const BufferRef& input, Buffer& output)
{
	if (filters.empty())
		return 0;

	if (filters.size() == 1)
		return filters.front()->process(input, output);

	auto i = filters.begin();
	auto e = filters.end();
	Buffer result;
	Buffer tmp;

	(*i++)->process(input, result);

	do {
		result.swap(tmp);
		(*i++)->process(tmp.ref(), result);
	} while (i != e);

	ssize_t n = result.size();

	if (output.capacity())
		output.push_back(result);
	else
		output = std::move(result);

	return n;
}

} // namespace xio
