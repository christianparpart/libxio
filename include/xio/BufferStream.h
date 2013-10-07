#pragma once

#include <xio/Stream.h>
#include <xio/Buffer.h>

namespace xio {

class XIO_API BufferStream : public Stream
{
private:
	Buffer data_;
	size_t readOffset_;

public:
	BufferStream() : data_(), readOffset_(0) {}
	explicit BufferStream(size_t cap);
	~BufferStream();

	void clear();
	virtual size_t size() const;
	size_t capacity() const { return data_.capacity(); }

	void shift(size_t n);

	// random access
	const char* data() const { return data_.data(); }
	const char& operator[](size_t n) const { return data_[n]; }

	// write to pipe
	virtual ssize_t write(const void* buf, size_t size);
	virtual ssize_t write(Socket* socket, size_t size, Mode mode);
	virtual ssize_t write(Pipe* pipe, size_t size, Mode mode);
	virtual ssize_t write(int fd, size_t size);

	// read from pipe
	virtual ssize_t read(void* buf, size_t size);
	virtual ssize_t read(Socket* socket, size_t size);
	virtual ssize_t read(Pipe* socket, size_t size);
	virtual ssize_t read(int fd, size_t size);
	virtual int read();

	virtual void accept(StreamVisitor&);

	size_t readOffset() const { return readOffset_; }
	size_t writeOffset() const { return data_.size(); }

private:
	char* rwdata() { return (char*) data_.data(); }
};

} // namespace xio
