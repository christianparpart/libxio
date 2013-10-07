#pragma once

#include <xio/Stream.h>

namespace xio {

class StreamReader
{
public:
	explicit StreamReader(Stream* stream) : stream_(stream) {}

	ssize_t read(char* buf, size_t size) { return stream_->read(buf, size); }
	ssize_t read(Socket* socket, size_t size) { return stream_->read(socket, size); }
	ssize_t read(Pipe* pipe, size_t size) { return stream_->read(pipe, size); }
	ssize_t read(int fd, size_t size) { return stream_->read(fd, size); }
	int read() { return stream_->read(); }

private:
	Stream* stream_;
};

} // namespace xio
