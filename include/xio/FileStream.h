#pragma once

#include <xio/Api.h>
#include <xio/Stream.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

namespace xio {

class XIO_API FileStream : public Stream
{
public:
	explicit FileStream(int fd);
	virtual ~FileStream();

	virtual size_t size() const;

	// TODO this is going to integrate legacy Source/Sink API
//	virtual ssize_t read(Stream* sink);

	virtual ssize_t read(void* buf, size_t size);
	virtual ssize_t read(Socket* socket, size_t size);
	virtual ssize_t read(Pipe* pipe, size_t size);
	virtual ssize_t read(int fd, size_t size);
	virtual int read();

	virtual ssize_t write(const void* buf, size_t size);
	virtual ssize_t write(Socket* socket, size_t size, Mode mode = Stream::MOVE);
	virtual ssize_t write(Pipe* pipe, size_t size, Mode mode = Stream::MOVE);
	virtual ssize_t write(int fd, size_t size);

	virtual void accept(StreamVisitor&);

protected:
	int fd_;
};

} // namespace xio
