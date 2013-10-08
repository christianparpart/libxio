#include <xio/FilterStream.h>
#include <xio/Filter.h>
#include <xio/Buffer.h>
#include <deque>

namespace xio {

FilterStream::FilterStream(std::unique_ptr<Stream> parent) :
	parent_(std::move(parent))
{
}

FilterStream::FilterStream(Stream* parent, std::unique_ptr<Filter> filter) :
	parent_(std::move(parent)),
	inputBuffer_(),
	inputPos_(0),
	outputBuffer_(),
	outputPos_(0),
	filters()
{
	filters.push_back(std::move(filter));
}

FilterStream::~FilterStream()
{
}

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

// {{{ Stream API impl
bool FilterStream::empty() const
{
	return parent_->empty();
}

size_t FilterStream::size() const
{
	return parent_->size();
}

ssize_t FilterStream::pull(size_t size)
{
	if (inputBuffer_.size() - inputPos_ >= size)
		return size;

	inputBuffer_.reserve(inputBuffer_.size() + size);

	ssize_t rv = parent_->read(inputBuffer_, size);

	return rv != -1
		? inputBuffer_.size() - inputPos_ + rv
		: -1;
}

ssize_t FilterStream::read(Buffer& result, size_t size)
{
	pull(size);

	process(inputBuffer_.ref(inputPos_), outputBuffer_);

	size = std::min(size, outputBuffer_.size() - outputPos_);

	size_t lsize = result.size();
	result.push_back(outputBuffer_.data() + outputPos_, size);
	return lsize - result.size();
}

ssize_t FilterStream::read(char* buf, size_t size)
{
	size = pull(size);

	process(inputBuffer_.ref(inputPos_), outputBuffer_);

	return size;
}

ssize_t FilterStream::read(Socket* socket, size_t size)
{
	return 0;
}

ssize_t FilterStream::read(Pipe* pipe, size_t size)
{
	return 0;
}

ssize_t FilterStream::read(int fd, size_t size)
{
	return 0;
}

int FilterStream::read()
{
	return 0;
}

ssize_t FilterStream::write(const char* buf, size_t size)
{
	return 0;
}

ssize_t FilterStream::write(Socket* socket, size_t size, Mode mode)
{
	return 0;
}

ssize_t FilterStream::write(Pipe* pipe, size_t size, Mode mode)
{
	return 0;
}

ssize_t FilterStream::write(int fd, size_t size)
{
	return 0;
}

void FilterStream::accept(StreamVisitor& v)
{
}

// }}}

} // namespace xio
