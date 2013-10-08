#pragma once

#include <xio/Api.h>
#include <xio/Stream.h>
#include <xio/Buffer.h>
#include <memory>
#include <deque>
#include <sys/types.h>
#include <stdint.h>
#include <unistd.h>

namespace xio {

class Filter;

class XIO_API FilterStream : public Stream
{
private:
	std::unique_ptr<Stream> parent_;
	Buffer inputBuffer_;
	size_t inputPos_;
	Buffer outputBuffer_;
	size_t outputPos_;

public:
	std::deque<std::unique_ptr<Filter>> filters;

public:
	explicit FilterStream(std::unique_ptr<Stream> parent);
	FilterStream(Stream* parent, std::unique_ptr<Filter> filter);
	~FilterStream();

public: // Stream API
	virtual bool empty() const;
	virtual size_t size() const;

	// TODO this is going to integrate legacy Source/Sink API
//	virtual ssize_t read(Stream* sink);

	ssize_t read(Buffer& result, size_t size);
	virtual ssize_t read(char* buf, size_t size);
	virtual ssize_t read(Socket* socket, size_t size);
	virtual ssize_t read(Pipe* pipe, size_t size);
	virtual ssize_t read(int fd, size_t size);
	virtual int read();

	virtual ssize_t write(const char* buf, size_t size);
	virtual ssize_t write(Socket* socket, size_t size, Mode mode = Stream::MOVE);
	virtual ssize_t write(Pipe* pipe, size_t size, Mode mode = Stream::MOVE);
	virtual ssize_t write(int fd, size_t size);

	virtual void accept(StreamVisitor&);

private:
	ssize_t pull(size_t size);
	ssize_t process(const BufferRef& input, Buffer& output);
};

} // namespace xio
