#include <xio/MemoryBuffer.h>
#include <xio/StreamVisitor.h>
#include <xio/KernelBuffer.h>
#include <algorithm>
#include <cstring>

// TODO socket functions

namespace xio {

MemoryBuffer::MemoryBuffer(size_t cap) :
	data_(new char[cap]),
	readOffset_(0),
	writeOffset_(0),
	capacity_(cap)
{
}

MemoryBuffer::~MemoryBuffer()
{
	delete[] data_;
}

size_t MemoryBuffer::size() const
{
	return writeOffset_ - readOffset_;
}

ssize_t MemoryBuffer::write(const void* buf, size_t size)
{
	size_t n = std::min(capacity_ - writeOffset_, size);
	memcpy(data_ + writeOffset_, buf, n);
	writeOffset_ += n;
	return n;
}

ssize_t MemoryBuffer::write(Socket* socket, size_t size, Mode /*mode*/)
{
	return 0; // TODO
}

ssize_t MemoryBuffer::write(KernelBuffer* pipe, size_t size, Mode /*mode*/)
{
	ssize_t n = std::min(capacity_ - writeOffset_, size);
	n = pipe->read(data_ + writeOffset_, n);

	if (n > 0)
		writeOffset_ += n;

	return n;
}

ssize_t MemoryBuffer::write(int fd, size_t size)
{
	ssize_t n = std::min(capacity_ - writeOffset_, size);
	n = ::read(fd, data_ + writeOffset_, n);

	if (n > 0)
		writeOffset_ += n;

	return n;
}

ssize_t MemoryBuffer::write(int fd, off_t *fd_off, size_t size)
{
	ssize_t n = std::min(capacity_ - writeOffset_, size);
	n = fd_off
		? ::pread(fd, data_ + writeOffset_, n, *fd_off)
		: ::read(fd, data_ + writeOffset_, n);

	if (n > 0)
		writeOffset_ += n;

	return n;
}

ssize_t MemoryBuffer::read(void* buf, size_t size)
{
	if (size < this->size()) {
		memcpy(buf, data_, size);
		readOffset_ += size;
	} else {
		memcpy(buf, data_, this->size());
		readOffset_ = writeOffset_ = 0;
	}

	return size;
}

ssize_t MemoryBuffer::read(Socket* socket, size_t size)
{
	return 0; // TODO
}

ssize_t MemoryBuffer::read(KernelBuffer* pipe, size_t size)
{
	ssize_t n = pipe->write(data_, std::min(this->size(), size));
	if (n > 0) {
		readOffset_ += n;
	}

	return n;
}

ssize_t MemoryBuffer::read(int fd, size_t size)
{
	ssize_t n = ::write(fd, data_, std::min(this->size(), size));
	if (n > 0) {
		readOffset_ += n;
	}

	return n;
}

ssize_t MemoryBuffer::read(int fd, off_t *fd_off, size_t size)
{
	ssize_t n = fd_off 
		? ::pwrite(fd, data_, std::min(this->size(), size), *fd_off)
		: ::write(fd, data_, std::min(this->size(), size));

	if (n > 0) {
		readOffset_ += n;
	}

	return n;
}

int MemoryBuffer::read()
{
	if (readOffset_ != writeOffset_) {
		int ch = data_[readOffset_];
		++readOffset_;
		return ch;
	}

	return -1;
}

void MemoryBuffer::accept(StreamVisitor& visitor)
{
	visitor.visit(*this);
}

} // namespace xio
