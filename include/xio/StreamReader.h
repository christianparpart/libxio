#pragma once

#include <xio/Stream.h>

namespace xio {

class StreamReader
{
public:
	explicit StreamReader(Stream* stream) : stream_(stream) {}

	ssize_t read(void* buf, size_t size) { return stream_->read(buf, size); }
	ssize_t read(Socket* socket, size_t size) { return stream_->read(socket, size); }
	ssize_t read(KernelBuffer* pipe, size_t size) { return stream_->read(pipe, size); }
	ssize_t read(int fd, size_t size) { return stream_->read(fd, size); }
	ssize_t read(int fd, off_t *fd_off, size_t size) { return stream_->read(fd, fd_off, size); }
	int read() { return stream_->read(); }

private:
	Stream* stream_;
};

} // namespace xio
