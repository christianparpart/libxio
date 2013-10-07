#include <xio/FileStream.h>
#include <xio/StreamVisitor.h>

namespace xio {

FileStream::FileStream(int fd) :
	fd_(fd)
{
}

FileStream::~FileStream()
{
	if (fd_ >= 0)
		::close(fd_);
}

size_t FileStream::size() const
{
	struct stat st;

	if (::fstat(fd_, &st) < 0)
		return 0;

	return st.st_size;
}

ssize_t FileStream::read(void* buf, size_t size)
{
	return ::read(fd_, buf, size);
}

ssize_t FileStream::read(Socket* socket, size_t size)
{
	return -1; // TODO
}

ssize_t FileStream::read(Pipe* pipe, size_t size)
{
	return -1; // TODO
}

ssize_t FileStream::read(int fd, size_t size)
{
	return -1; // TODO
}

int FileStream::read()
{
	return -1; // TODO
}

ssize_t FileStream::write(const void* buf, size_t size)
{
	return ::write(fd_, buf, size);
}

ssize_t FileStream::write(Socket* socket, size_t size, Mode mode)
{
	return -1; // TODO
}

ssize_t FileStream::write(Pipe* pipe, size_t size, Mode mode)
{
	return -1; // TODO
}

ssize_t FileStream::write(int fd, size_t size)
{
	return -1; // TODO
}

void FileStream::accept(StreamVisitor& visitor)
{
	visitor.visit(*this);
}

} // namespace xio
