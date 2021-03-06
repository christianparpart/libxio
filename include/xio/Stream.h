#pragma once

#include <xio/Api.h>
#include <sys/types.h>
#include <stdint.h>
#include <unistd.h>

namespace xio {

class Buffer;
class Socket;
class Pipe;
class FileStream;
class StreamVisitor;

class XIO_API Stream
{
public:
	enum Mode {
		COPY, //!< writes by copying all input data into this chunk
		MOVE, //!< transfers data into chunk by attempting to move kernel buffers, but falls back to copy if not supported.
	};

	virtual ~Stream() {}

	virtual bool empty() const;
	virtual size_t size() const = 0;

	// TODO this is going to integrate legacy Source/Sink API
//	virtual ssize_t read(Stream* sink) = 0;

	virtual ssize_t read(Buffer& result, size_t size) = 0;
	virtual ssize_t read(char* buf, size_t size) = 0;
	virtual ssize_t read(Socket* socket, size_t size) = 0;
	virtual ssize_t read(Pipe* pipe, size_t size) = 0;
	virtual ssize_t read(int fd, size_t size) = 0;
	virtual int read() = 0;

	virtual ssize_t write(const char* buf, size_t size) = 0;
	virtual ssize_t write(Socket* socket, size_t size, Mode mode = Stream::MOVE) = 0;
	virtual ssize_t write(Pipe* pipe, size_t size, Mode mode = Stream::MOVE) = 0;
	virtual ssize_t write(int fd, size_t size) = 0;
	virtual ssize_t write(const char* str);

	virtual void accept(StreamVisitor&) = 0;
};

} // namespace xio
