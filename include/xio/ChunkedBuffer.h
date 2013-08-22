#pragma once

#include <xio/Stream.h>
#include <xio/StreamReader.h>

#include <sys/types.h>
#include <cstdint>
#include <cstring>
#include <utility>
#include <deque>

namespace xio {

class XIO_API ChunkedBuffer : public Stream
{
public:
	ChunkedBuffer();
	~ChunkedBuffer();

	virtual bool empty() const;
	virtual size_t size() const;

	// write to buffer
	virtual ssize_t write(const void* buf, size_t size);
	virtual ssize_t write(Socket* socket, size_t size, Mode mode);
	virtual ssize_t write(KernelBuffer* pipe, size_t size, Mode mode);
	virtual ssize_t write(int fd, off_t *fd_off, size_t size);
	virtual ssize_t write(int fd, size_t size);

	// read from buffer
	virtual ssize_t read(void* buf, size_t size);
	virtual ssize_t read(Socket* socket, size_t size);
	virtual ssize_t read(KernelBuffer* socket, size_t size);
	virtual ssize_t read(int fd, size_t size);
	virtual ssize_t read(int fd, off_t *fd_off, size_t size);
	virtual int read();

	virtual void accept(StreamVisitor&);

	StreamReader front() const { return !chunks_.empty() ? StreamReader(chunks_.front()) : StreamReader(nullptr); }
	void pop_front();

private:
	Stream* buffer(size_t size);
	Stream* pipe(size_t size);

	std::deque<Stream*> chunks_;
};

// {{{ inlines
inline ChunkedBuffer::ChunkedBuffer() :
	chunks_()
{}

inline ChunkedBuffer::~ChunkedBuffer()
{
	for (auto chunk: chunks_)
		delete chunk;
}
// }}}

} // namespace xioo 
