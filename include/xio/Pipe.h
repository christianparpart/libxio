#pragma once
/* <Pipe.h>
 *
 * This file is part of the x0 web server project and is released under LGPL-3.
 * http://www.xzero.io/
 *
 * (c) 2009-2013 Christian Parpart <trapni@gmail.com>
 */

#include <xio/Stream.h>
#include <sys/types.h>
#include <unistd.h>

namespace xio {

class Socket;

//! \addtogroup io
//@{

class XIO_API Pipe : public Stream
{
private:
	int pipe_[2];
	size_t size_; // number of bytes available in pipe

public:
	explicit Pipe(int flags = 0);
	~Pipe();

	bool isOpen() const;

	virtual size_t size() const;
	bool isEmpty() const;

	void clear();

	// direct access to their internal file descriptors
	int writeFd() const;
	int readFd() const;

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
};

//@}

// {{{ impl
inline bool Pipe::isOpen() const
{
	return pipe_[0] >= 0;
}

inline int Pipe::writeFd() const
{
	return pipe_[1];
}

inline int Pipe::readFd() const
{
	return pipe_[0];
}

inline bool Pipe::isEmpty() const
{
	return size_ == 0;
}
// }}}

} // namespace xio
