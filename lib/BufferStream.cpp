#include <xio/BufferStream.h>
#include <xio/StreamVisitor.h>
#include <xio/Pipe.h>
#include <algorithm>
#include <cstring>

// TODO socket functions

namespace xio {

BufferStream::BufferStream(size_t cap) :
	data_(cap),
	readOffset_(0),
	writeOffset_(0)
{
}

BufferStream::~BufferStream()
{
}

size_t BufferStream::size() const
{
	return writeOffset_ - readOffset_;
}

ssize_t BufferStream::write(const void* buf, size_t size)
{
	size_t n = std::min(capacity() - writeOffset_, size);
	memcpy(data() + writeOffset_, buf, n);
	writeOffset_ += n;
	return n;
}

ssize_t BufferStream::write(Socket* socket, size_t size, Mode /*mode*/)
{
	return 0; // TODO
}

ssize_t BufferStream::write(Pipe* pipe, size_t size, Mode /*mode*/)
{
	ssize_t n = std::min(capacity() - writeOffset_, size);
	n = pipe->read(data() + writeOffset_, n);

	if (n > 0)
		writeOffset_ += n;

	return n;
}

ssize_t BufferStream::write(int fd, size_t size)
{
	ssize_t n = std::min(capacity() - writeOffset_, size);
	n = ::read(fd, data() + writeOffset_, n);

	if (n > 0)
		writeOffset_ += n;

	return n;
}

ssize_t BufferStream::write(int fd, off_t *fd_off, size_t size)
{
	ssize_t n = std::min(capacity() - writeOffset_, size);
	n = fd_off
		? ::pread(fd, data() + writeOffset_, n, *fd_off)
		: ::read(fd, data() + writeOffset_, n);

	if (n > 0)
		writeOffset_ += n;

	return n;
}

ssize_t BufferStream::read(void* buf, size_t size)
{
	if (size < this->size()) {
		memcpy(buf, data(), size);
		readOffset_ += size;
	} else {
		memcpy(buf, data(), this->size());
		readOffset_ = writeOffset_ = 0;
	}

	return size;
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
	if (readOffset_ != writeOffset_) {
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
