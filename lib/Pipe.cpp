/* <src/Pipe.cpp>
 *
 * This file is part of the x0 web server project and is released under GPL-3.
 * http://www.xzero.io/
 *
 * (c) 2009-2013 Christian Parpart <trapni@gmail.com>
 */

#include <xio/Pipe.h>
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
Pipe::Pipe(int flags) :
	size_(0)
{
	if (::pipe2(pipe_, flags) < 0) {
		pipe_[0] = -errno;
		pipe_[1] = -1;
	}
}

Pipe::~Pipe()
{
	if (isOpen()) {
		::close(pipe_[0]);
		::close(pipe_[1]);
	}
}

size_t Pipe::size() const
{
	return size_;
}

void Pipe::clear()
{
	char buf[4096];
	while (size_ > 0) {
		ssize_t rv = ::read(readFd(), buf, std::min(size_, sizeof(buf)));
		if (rv > 0) {
			size_ -= rv;
		}
	}
}

ssize_t Pipe::write(const void* buf, size_t size)
{
	ssize_t rv = ::write(writeFd(), buf, size);

	if (rv > 0)
		size_ += rv;

	return rv;
}

ssize_t Pipe::write(Socket* socket, size_t size, Mode mode)
{
	return 0;//socket->write(this, size);
}

ssize_t Pipe::write(Pipe* pipe, size_t size, Mode mode)
{
	if (mode == MOVE) {
		ssize_t rv = splice(pipe->readFd(), NULL, writeFd(), NULL, pipe->size_, SPLICE_F_MOVE | SPLICE_F_NONBLOCK);
		if (rv > 0) {
			pipe->size_ -= rv;
			size_ += rv;
		}
		return rv;
	} else {
		ssize_t rv = tee(pipe->readFd(), writeFd(), size, SPLICE_F_NONBLOCK);
		if (rv > 0) {
			size_ += rv;
		}
		return rv;
	}
}

ssize_t Pipe::write(int fd, size_t size)
{
	ssize_t rv = splice(fd, NULL, writeFd(), NULL, size, SPLICE_F_MOVE | SPLICE_F_NONBLOCK);

	if (rv > 0)
		size_ += rv;

	return rv;
}

ssize_t Pipe::read(void* buf, size_t size)
{
	ssize_t rv = ::read(readFd(), buf, size);

	if (rv > 0)
		size_ -= rv;

	return rv;
}

ssize_t Pipe::read(Socket* socket, size_t size)
{
	return 0;//socket->read(this, size);
}

ssize_t Pipe::read(Pipe* pipe, size_t size)
{
	return pipe->write(this, size);
}

ssize_t Pipe::read(int fd, size_t size)
{
	ssize_t rv = splice(readFd(), NULL, fd, NULL, size, SPLICE_F_MOVE | SPLICE_F_NONBLOCK);

	if (rv > 0)
		size_ -= rv;

	return rv;
}

int Pipe::read()
{
	if (size_ != 0) {
		char ch = 0;
		if (read(&ch, 1) == 1) {
			return ch;
		}
	}

	return -1;
}

void Pipe::accept(StreamVisitor& visitor)
{
	visitor.visit(*this);
}

} // namespace xio
