#pragma once

#include <xio/Stream.h>

namespace xio {

class XIO_API MemoryBuffer : public Stream
{
private:
	char* data_;
	size_t readOffset_;
	size_t writeOffset_;
	size_t capacity_;

public:
	MemoryBuffer() : data_(nullptr), readOffset_(0), writeOffset_(0), capacity_(0) {}
	explicit MemoryBuffer(size_t cap);
	~MemoryBuffer();

	void clear();
	virtual size_t size() const;
	size_t capacity() const { return capacity_; }

	// random access
	const char* data() const { return data_; }
	const char& operator[](size_t n) const { return data_[n]; }

	// write to pipe
	virtual ssize_t write(const void* buf, size_t size);
	virtual ssize_t write(Socket* socket, size_t size, Mode mode);
	virtual ssize_t write(KernelBuffer* pipe, size_t size, Mode mode);
	virtual ssize_t write(int fd, off_t *fd_off, size_t size);
	virtual ssize_t write(int fd, size_t size);

	// read from pipe
	virtual ssize_t read(void* buf, size_t size);
	virtual ssize_t read(Socket* socket, size_t size);
	virtual ssize_t read(KernelBuffer* socket, size_t size);
	virtual ssize_t read(int fd, size_t size);
	virtual ssize_t read(int fd, off_t *fd_off, size_t size);
	virtual int read();

	virtual void accept(StreamVisitor&);
};

} // namespace xio
