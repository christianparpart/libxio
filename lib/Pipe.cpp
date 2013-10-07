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

ssize_t Pipe::write(const char* buf, size_t size)
{
	ssize_t rv = ::write(writeFd(), buf, size);

	if (rv > 0)
		size_ += rv;

	return rv;
}

ssize_t Pipe::write(Socket* socket, size_t size, Mode mode)
{
	if (mode != MOVE) {
		errno = EINVAL;
		return -1;
	}

	ssize_t rv = splice(socket->handle(), NULL, writeFd(), NULL, size, SPLICE_F_MOVE | SPLICE_F_NONBLOCK);
	if (rv > 0)
		size_ += rv;

	return rv;
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

ssize_t Pipe::read(Buffer& result, size_t size)
{
	ssize_t nread = 0;

	while (size > 0) {
		if (result.capacity() - result.size() < 256) {
			if (!result.reserve(std::max(static_cast<std::size_t>(4096), static_cast<std::size_t>(result.size() * 1.5)))) {
				// could not allocate enough memory
				errno = ENOMEM;
				return nread ? nread : -1;
			}
		}

		size_t nbytes = result.capacity() - result.size();
		nbytes = std::min(nbytes, size);

		ssize_t rv = ::read(readFd(), result.end(), nbytes);
		if (rv <= 0) {
			return nread != 0 ? nread : rv;
		} else {
			size -= rv;
			nread += rv;
			result.resize(result.size() + rv);

			if (static_cast<std::size_t>(rv) < nbytes) {
				return nread;
			}
		}
	}

	return nread;
}
ssize_t Pipe::read(char* buf, size_t size)
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
	return 0; //pipe->write(this, size);
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
