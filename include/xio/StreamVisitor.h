#pragma once

#include <xio/Api.h>

namespace xio {

class MemoryBuffer;
class KernelBuffer;
class ChunkedBuffer;
class File;
class Socket;

class XIO_API StreamVisitor
{
public:
	virtual void visit(MemoryBuffer&) = 0;
	virtual void visit(KernelBuffer&) = 0;
	virtual void visit(ChunkedBuffer&) = 0;
	virtual void visit(File&) = 0;
	virtual void visit(Socket&) = 0;
};

} // namespace xio
