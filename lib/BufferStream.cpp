#include <xio/BufferStream.h>
#include <xio/StreamVisitor.h>
#include <xio/Pipe.h>
#include <algorithm>
#include <cstring>

// TODO socket functions

namespace xio {

BufferStream::BufferStream(size_t cap) :
	data_(cap),
	readOffset_(0)
{
}

BufferStream::~BufferStream()
{
}

size_t BufferStream::size() const
{
	return writeOffset() - readOffset_;
}

void BufferStream::shift(size_t n)
{
	if (readOffset_ + n == writeOffset()) {
		readOffset_ += n;
	} else {
		readOffset_ = 0;
		data_.clear();
	}
}

ssize_t BufferStream::write(const void* buf, size_t size)
{
	size_t n = data_.size();
	data_.push_back(buf, size);
	return data_.size() - n;
}

ssize_t BufferStream::write(Socket* socket, size_t size, Mode /*mode*/)
{
	return 0; // TODO
}

ssize_t BufferStream::write(Pipe* pipe, size_t size, Mode /*mode*/)
{
	data_.reserve(data_.size() + size);

	ssize_t n = std::min(capacity() - writeOffset(), size);
	n = pipe->read(data() + writeOffset(), n);

	if (n > 0)
		data_.resize(data_.size() + n);

	return n;
}

ssize_t BufferStream::write(int fd, size_t size)
{
	data_.reserve(data_.size() + size);

	ssize_t n = std::min(capacity() - writeOffset(), size);
	n = ::read(fd, data() + writeOffset(), n);

	if (n > 0)
		data_.resize(data_.size() + n);

	return n;
}

ssize_t BufferStream::write(int fd, off_t *fd_off, size_t size)
{
	data_.reserve(data_.size() + size);

	ssize_t n = std::min(capacity() - writeOffset(), size);
	n = fd_off
		? ::pread(fd, data() + writeOffset(), n, *fd_off)
		: ::read(fd, data() + writeOffset(), n);

	if (n > 0)
		data_.resize(data_.size() + n);

	return n;
}

ssize_t BufferStream::read(void* buf, size_t size)
{
	ssize_t n = std::min(size, this->size());
	memcpy(buf, data(), n);

	shift(n);

	return n;
}

ssize_t BufferStream::read(Socket* socket, size_t size)
{
	return 0; // TODO
}

ssize_t BufferStream::read(Pipe* pipe, size_t size)
{
	ssize_t n = pipe->write(data(), std::min(this->size(), size));
	if (n > 0) {
		readOffset_ += n;
	}

	return n;
}

ssize_t BufferStream::read(int fd, size_t size)
{
	ssize_t n = ::write(fd, data(), std::min(this->size(), size));
	if (n > 0) {
		readOffset_ += n;
	}

	return n;
}

ssize_t BufferStream::read(int fd, off_t *fd_off, size_t size)
{
	ssize_t n = fd_off 
		? ::pwrite(fd, data(), std::min(this->size(), size), *fd_off)
		: ::write(fd, data(), std::min(this->size(), size));

	if (n > 0) {
		readOffset_ += n;
	}

	return n;
}

int BufferStream::read()
{
	if (readOffset_ != writeOffset()) {
		int ch = data()[readOffset_];
		++readOffset_;
		return ch;
	}

	return -1;
}

void BufferStream::accept(StreamVisitor& visitor)
{
	visitor.visit(*this);
}

} // namespace xio
