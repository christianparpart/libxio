#pragma once

#include <xio/Api.h>
#include <xio/Stream.h>
#include <memory>
#include <deque>
#include <sys/types.h>
#include <stdint.h>
#include <unistd.h>

namespace xio {

class Filter;
class BufferRef;
class Buffer;

class XIO_API FilterStream : public Stream
{
private:
	Stream* parent_;

public:
	std::deque<std::unique_ptr<Filter>> filters;

public:
	explicit FilterStream(Stream* parent);
	FilterStream(Stream* parent, std::unique_ptr<Filter> filter);
	~FilterStream();

public: // Stream API
	virtual bool empty() const;
	virtual size_t size() const;

	// TODO this is going to integrate legacy Source/Sink API
//	virtual ssize_t read(Stream* sink);

	virtual ssize_t read(void* buf, size_t size);
	virtual ssize_t read(Socket* socket, size_t size);
	virtual ssize_t read(Pipe* pipe, size_t size);
	virtual ssize_t read(int fd, size_t size);
	virtual ssize_t read(int fd, off_t *fd_off, size_t size);
	virtual int read();

	virtual ssize_t write(const void* buf, size_t size);
	virtual ssize_t write(Socket* socket, size_t size, Mode mode = Stream::MOVE);
	virtual ssize_t write(Pipe* pipe, size_t size, Mode mode = Stream::MOVE);
	virtual ssize_t write(int fd, off_t *fd_off, size_t size);
	virtual ssize_t write(int fd, size_t size);

	virtual void accept(StreamVisitor&);

	ssize_t process(const BufferRef& input, Buffer& output);
};

} // namespace xio
