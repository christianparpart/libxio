#include <xio/ChunkedBuffer.h>
#include <xio/MemoryBuffer.h>
#include <xio/KernelBuffer.h>
#include <xio/StreamVisitor.h>

#include <fcntl.h>

namespace xio {

bool ChunkedBuffer::empty() const
{
	if (chunks_.empty())
		return true;

	return chunks_.front()->size() == 0;
}

size_t ChunkedBuffer::size() const
{
	size_t result = 0;

	for (auto chunk: chunks_)
		result += chunk->size();

	return result;
}

ssize_t ChunkedBuffer::write(const void* buf, size_t size)
{
	if (auto chunk = buffer(size))
		return chunk->write(buf, size);

	return -1;
}

ssize_t ChunkedBuffer::write(Socket* socket, size_t size, Mode mode)
{
	return 0; // TODO
}

ssize_t ChunkedBuffer::write(KernelBuffer* pp, size_t size, Mode mode)
{
	if (mode == Stream::MOVE) {
		if (auto chunk = pipe(size)) {
			return chunk->write(pp, size);
		}
	}

	if (auto chunk = buffer(size)) {
		return chunk->write(pp, size);
	}

	return -1;
}

ssize_t ChunkedBuffer::write(int fd, size_t size)
{
	if (auto chunk = pipe(size))
		return chunk->write(fd, size);

	return -1;
}

ssize_t ChunkedBuffer::write(int fd, off_t *fd_off, size_t size)
{
	if (auto chunk = buffer(size))
		return chunk->write(fd, fd_off, size);

	return -1;
}

ssize_t ChunkedBuffer::read(void* buf, size_t size)
{
	return 0;
}

ssize_t ChunkedBuffer::read(Socket* socket, size_t size)
{
	return 0; // TODO
}

ssize_t ChunkedBuffer::read(KernelBuffer* pp, size_t size)
{
	ssize_t result = 0;
	while (!empty() && size > 0) {
		auto chunk = chunks_.front();
		auto n = chunk->read(pp, size);

		if (n < 0)
			return result ? result : -1;

		if (n > 0) {
			result += n;
			size -= n;
		}

		if (chunk->size() == 0) {
			chunks_.pop_front();
			delete chunk;
		}
	}
	return result;
}

ssize_t ChunkedBuffer::read(int fd, size_t size)
{
	size_t result = 0;
	while (!empty() && size > 0) {
		auto chunk = chunks_.front();
		ssize_t n = chunk->read(fd, size);

		if (n < 0)
			return result ? result : -1;

		if (n > 0) {
			result += n;
			size -= n;
		}

		if (chunk->size() == 0) {
			chunks_.pop_front();
			delete chunk;
		}
	}

	return result;
}

ssize_t ChunkedBuffer::read(int fd, off_t *fd_off, size_t size)
{
	return 0;
}

int ChunkedBuffer::read()
{
	if (!empty()) {
		auto chunk = chunks_.front();
		auto ch = chunk->read();
		if (chunk->size() == 0)
			pop_front();

		if (ch != -1)
			return ch;
	}

	return -1;
}

void ChunkedBuffer::pop_front()
{
	delete chunks_.front();
	chunks_.pop_front();
}

Stream* ChunkedBuffer::buffer(size_t size)
{
	if (!chunks_.empty()) {
		if (auto tail = dynamic_cast<MemoryBuffer*>(chunks_.back())) {
			if (tail->size() < tail->capacity()) {
				return tail;
			}
		}
	}

	if (auto c = new MemoryBuffer(size)) {
		chunks_.push_back(c);
		return c;
	}

	return nullptr;
}

Stream* ChunkedBuffer::pipe(size_t size)
{
	if (!chunks_.empty()) {
		if (auto tail = dynamic_cast<KernelBuffer*>(chunks_.back())) {
			// TODO ensure size' capacity availability
			return tail;
		}
	}

	if (auto c = new KernelBuffer(O_NONBLOCK | O_CLOEXEC)) {
		chunks_.push_back(c);
		return c;
	}

	return nullptr;
}

void ChunkedBuffer::accept(StreamVisitor& visitor)
{
	visitor.visit(*this);
}

} // namespace xio
