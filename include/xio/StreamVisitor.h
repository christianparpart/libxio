#pragma once

#include <xio/Api.h>

namespace xio {

class Pipe;
class BufferStream;
class ChunkedStream;
class FileStream;
class Socket;

class XIO_API StreamVisitor
{
public:
	virtual void visit(Pipe&) = 0;
	virtual void visit(BufferStream&) = 0;
	virtual void visit(ChunkedStream&) = 0;
	virtual void visit(FileStream&) = 0;
	virtual void visit(Socket&) = 0;
};

} // namespace xio
