/* <src/KernelBuffer.cpp>
 *
 * This file is part of the x0 web server project and is released under GPL-3.
 * http://www.xzero.io/
 *
 * (c) 2009-2013 Christian Parpart <trapni@gmail.com>
 */

#include <xio/KernelBuffer.h>
#include <xio/StreamVisitor.h>
#include <xio/Socket.h>

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

namespace xio {

/** Creates a pipe.
 *
 * @param flags an OR'ed value of O_NONBLOCK and O_CLOEXEC
 */
KernelBuffer::KernelBuffer(int flags) :
	size_(0)
{
	if (::pipe2(pipe_, flags) < 0) {
		pipe_[0] = -errno;
		pipe_[1] = -1;
	}
}

KernelBuffer::~KernelBuffer()
{
	if (isOpen()) {
		::close(pipe_[0]);
		::close(pipe_[1]);
	}
}

size_t KernelBuffer::size() const
{
	return size_;
}

void KernelBuffer::clear()
{
	char buf[4096];
	ssize_t rv;

	do rv = ::read(readFd(), buf, sizeof(buf));
	while (rv > 0);

	size_ = 0;
}

ssize_t KernelBuffer::write(const void* buf, size_t size)
{
	ssize_t rv = ::write(writeFd(), buf, size);

	if (rv > 0)
		size_ += rv;

	return rv;
}

ssize_t KernelBuffer::write(Socket* socket, size_t size, Mode mode)
{
	return 0;//socket->write(this, size);
}

ssize_t KernelBuffer::write(KernelBuffer* pipe, size_t size, Mode mode)
{
	ssize_t rv = splice(pipe->readFd(), NULL, writeFd(), NULL, pipe->size_, SPLICE_F_MOVE | SPLICE_F_NONBLOCK);

	if (rv > 0) {
		pipe->size_ -= rv;
		size_ += rv;
	}

	return rv;
}

ssize_t KernelBuffer::write(int fd, off_t* fd_off, size_t size)
{
	ssize_t rv = splice(fd, fd_off, writeFd(), NULL, size, SPLICE_F_MOVE | SPLICE_F_NONBLOCK);

	if (rv > 0)
		size_ += rv;

	return rv;
}

ssize_t KernelBuffer::write(int fd, size_t size)
{
	return write(fd, NULL, size);
}

ssize_t KernelBuffer::read(void* buf, size_t size)
{
	ssize_t rv = ::read(readFd(), buf, size);

	if (rv > 0)
		size_ -= rv;

	return rv;
}

ssize_t KernelBuffer::read(Socket* socket, size_t size)
{
	return 0;//socket->read(this, size);
}

ssize_t KernelBuffer::read(KernelBuffer* pipe, size_t size)
{
	return pipe->write(this, size);
}

ssize_t KernelBuffer::read(int fd, off_t* fd_off, size_t size)
{
	ssize_t rv = splice(readFd(), fd_off, fd, NULL, size, SPLICE_F_MOVE | SPLICE_F_NONBLOCK);

	if (rv > 0)
		size_ -= rv;

	return rv;
}

ssize_t KernelBuffer::read(int fd, size_t size)
{
	return read(fd, NULL, size);
}

int KernelBuffer::read()
{
	if (size_ != 0) {
		char ch = 0;
		if (read(&ch, 1) == 1) {
			return ch;
		}
	}

	return -1;
}

void KernelBuffer::accept(StreamVisitor& visitor)
{
	visitor.visit(*this);
}

} // namespace xio
