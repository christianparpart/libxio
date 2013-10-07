#pragma once
/* <xio/Buffer.h>
 *
 * This file is part of the xio web server project and is released under LGPL-3.
 * http://www.xzero.io/
 *
 * (c) 2009-2013 Christian Parpart <trapni@gmail.com>
 */

#include <xio/Api.h>
#include <cstddef>
#include <climits>
#include <cstring>
#include <cstdlib>
#include <cassert>
#include <string>
#include <stdexcept>
#include <cstdarg>

namespace xio {

//! \addtogroup base
//@{

template<typename> class BufferBase;
template<bool (*ensure)(void*, size_t)> class MutableBuffer;
class BufferRef;
class BufferSlice;
class FixedBuffer;
class Buffer;

// {{{ BufferTraits
template<typename T> struct BufferTraits;

template<>
struct BufferTraits<char*> {
	typedef char value_type;
	typedef char& reference_type;
	typedef char* pointer_type;
	typedef char* iterator;
	typedef const char* const_iterator;
	typedef char* data_type;
};

struct BufferOffset { // {{{
	mutable Buffer* buffer_;
	size_t offset_;

	BufferOffset() :
		buffer_(nullptr),
		offset_(0)
	{
	}

	BufferOffset(Buffer* buffer, size_t offset) :
		buffer_(buffer),
		offset_(offset)
	{
	}

	BufferOffset(const BufferOffset& v) :
		buffer_(v.buffer_),
		offset_(v.offset_)
	{
	}

	BufferOffset(BufferOffset&& v) :
		buffer_(v.buffer_),
		offset_(v.offset_)
	{
		v.buffer_ = nullptr;
		v.offset_ = 0;
	}

	BufferOffset& operator=(const BufferOffset& v)
	{
		buffer_ = v.buffer_;
		offset_ = v.offset_;

		return *this;
	}

	Buffer* buffer() const { return buffer_; }
	size_t offset() const { return offset_; }

	operator const char* () const;
	operator char* ();
}; // }}}

template<>
struct BufferTraits<Buffer> {
	typedef char value_type;
	typedef char& reference_type;
	typedef char* pointer_type;
	typedef char* iterator;
	typedef const char* const_iterator;
	typedef BufferOffset data_type;
};
// }}}
// {{{ BufferBase<T>
template<typename T>
class XIO_API BufferBase
{
public:
	typedef typename BufferTraits<T>::value_type value_type;
	typedef typename BufferTraits<T>::reference_type reference_type;
	typedef typename BufferTraits<T>::pointer_type pointer_type;
	typedef typename BufferTraits<T>::iterator iterator;
	typedef typename BufferTraits<T>::const_iterator const_iterator;
	typedef typename BufferTraits<T>::data_type data_type;

	enum { npos = size_t(-1) };

protected:
	data_type data_;
	size_t size_;

public:
	BufferBase() : data_(), size_(0) {}
	BufferBase(data_type data, size_t size) : data_(data), size_(size) {}
	BufferBase(const BufferBase<T>& v) : data_(v.data_), size_(v.size_) {}

	// properties
	pointer_type data() { return data_; }
	const pointer_type data() const { return const_cast<BufferBase<T>*>(this)->data_; }
	reference_type at(size_t offset) { return data()[offset]; }
	const reference_type at(size_t offset) const { return data()[offset]; }
	size_t size() const { return size_; }

	bool empty() const { return size_ == 0; }
	operator bool() const { return size_ != 0; }
	bool operator!() const { return size_ == 0; }

	void clear() { size_ = 0; }

	// iterator access
	iterator begin() const { return const_cast<BufferBase<T>*>(this)->data_; }
	iterator end() const { return const_cast<BufferBase<T>*>(this)->data_ + size_; }

	const_iterator cbegin() const { return data(); }
	const_iterator cend() const { return data() + size(); }

	// find
	template<typename PodType, size_t N>
	size_t find(PodType (&value)[N], size_t offset = 0) const;
	size_t find(const value_type *value, size_t offset = 0) const;
	size_t find(const BufferRef& value, size_t offset = 0) const;
	size_t find(value_type value, size_t offset = 0) const;

	// rfind
	template<typename PodType, size_t N> size_t rfind(PodType (&value)[N]) const;
	size_t rfind(const value_type *value) const;
	size_t rfind(const BufferRef& value) const;
	size_t rfind(value_type value) const;
	size_t rfind(value_type value, size_t offset) const;

	// contains
	bool contains(const BufferBase<T>& ref) const;

	// split
	std::pair<BufferRef, BufferRef> split(char delimiter) const;
	std::pair<BufferRef, BufferRef> split(const value_type* delimiter) const;

	// begins / ibegins
	bool begins(const BufferRef& value) const;
	bool begins(const value_type* value) const;
	bool begins(const std::string& value) const;
	bool begins(value_type value) const;

	bool ibegins(const BufferRef& value) const;
	bool ibegins(const value_type* value) const;
	bool ibegins(const std::string& value) const;
	bool ibegins(value_type value) const;

	// ends / iends
	bool ends(const BufferRef& value) const;
	bool ends(const value_type* value) const;
	bool ends(const std::string& value) const;
	bool ends(value_type value) const;

	bool iends(const BufferRef& value) const;
	bool iends(const value_type* value) const;
	bool iends(const std::string& value) const;
	bool iends(value_type value) const;

	// sub
	BufferRef ref(size_t offset = 0) const;
	BufferRef ref(size_t offset, size_t size) const;

	// mutation
	BufferRef chomp() const;
	BufferRef trim() const;

	// STL string
	std::string str() const;
	std::string substr(size_t offset) const;
	std::string substr(size_t offset, size_t count) const;

	// casts
	template<typename U> U hex() const;

	bool toBool() const;
	int toInt() const;
	double toDouble() const;
	float toFloat() const;

	void dump(const char *description = nullptr) const;
	static void dump(const void *bytes, std::size_t length, const char *description);
};

template<typename T> bool equals(const BufferBase<T>& a, const BufferBase<T>& b);
template<typename T> bool equals(const BufferBase<T>& a, const std::string& b);
template<typename T, typename PodType, std::size_t N> bool equals(const BufferBase<T>& a, PodType (&b)[N]);
template<typename T> bool equals(const std::string& a, const std::string& b);

template<typename T> bool iequals(const BufferBase<T>& a, const BufferBase<T>& b);
template<typename T> bool iequals(const BufferBase<T>& a, const std::string& b);
template<typename T, typename PodType, std::size_t N> bool iequals(const BufferBase<T>& a, PodType (&b)[N]);
template<typename T> bool iequals(const std::string& a, const std::string& b);

template<typename T> bool operator==(const BufferBase<T>& a, const BufferBase<T>& b);
template<typename T> bool operator==(const BufferBase<T>& a, const std::string& b);
template<typename T, typename PodType, std::size_t N> bool operator==(const BufferBase<T>& a, PodType (&b)[N]);
template<typename T, typename PodType, std::size_t N> bool operator==(PodType (&b)[N], const BufferBase<T>& a);

template<typename T> bool operator!=(const BufferBase<T>& a, const BufferBase<T>& b) { return !(a == b); }
template<typename T> bool operator!=(const BufferBase<T>& a, const std::string& b) { return !(a == b); }
template<typename T, typename PodType, std::size_t N> bool operator!=(const BufferBase<T>& a, PodType (&b)[N]) { return !(a == b); }
template<typename T, typename PodType, std::size_t N> bool operator!=(PodType (&b)[N], const BufferBase<T>& a) { return !(a == b); }
// }}}
// {{{ BufferRef
class XIO_API BufferRef : public BufferBase<char*>
{
public:
	BufferRef() : BufferBase<char*>() {}
	BufferRef(const char* data, size_t size) : BufferBase<char*>((data_type) data, size) {}
	BufferRef(const BufferRef& v) : BufferBase<char*>(v) {}
	template<typename PodType, size_t N> explicit BufferRef(PodType (&value)[N]) :
		BufferBase<char*>((data_type)value, N - 1) {}

	BufferRef& operator=(const BufferRef& v);
};
// }}}
// {{{ MutableBuffer

inline bool immutableEnsure(void* self, size_t size);
inline bool mutableEnsure(void* self, size_t size);

/**
 * \brief Fixed size unmanaged mutable buffer.
 *
 * @param ensure function invoked to ensure that enough space is available to write to.
 */
template<bool (*ensure)(void*, size_t)>
class XIO_API MutableBuffer :
	public BufferRef
{
protected:
	size_t capacity_;

public:
	MutableBuffer();
	MutableBuffer(char* value, size_t capacity);
	MutableBuffer(char* value, size_t capacity, size_t size);
	MutableBuffer(MutableBuffer&& v);

	void swap(MutableBuffer<ensure>& v);

	bool resize(size_t value);

	size_t capacity() const;
	bool operator!() const;

	bool reserve(size_t value);

	// buffer builders
	void push_back(value_type value);
	void push_back(int value);
	void push_back(long value);
	void push_back(long long value);
	void push_back(unsigned value);
	void push_back(unsigned long value);
	void push_back(unsigned long long value);
	void push_back(const BufferRef& value);
	void push_back(const BufferRef& value, size_t offset, size_t length);
	void push_back(const std::string& value);
	void push_back(const void *value, size_t size);
	template<typename PodType, size_t N> void push_back(PodType (&value)[N]);

	MutableBuffer& vprintf(const char* fmt, va_list args);

	MutableBuffer& printf(const char* fmt);

	template<typename... Args>
	MutableBuffer& printf(const char* fmt, Args... args);

	// random access
	reference_type operator[](size_t index);
	const reference_type operator[](size_t index) const;

	const value_type *c_str() const;

	static MutableBuffer fromCopy(const value_type *data, size_t count);
};
// }}}
// {{{ FixedBuffer
class FixedBuffer :
	public MutableBuffer<immutableEnsure>
{
public:
	FixedBuffer();
	FixedBuffer(char* data, size_t size);
	FixedBuffer(char* data, size_t capacity, size_t size);
};

inline FixedBuffer::FixedBuffer() :
	MutableBuffer<immutableEnsure>()
{
}

inline FixedBuffer::FixedBuffer(char* data, size_t size) :
	MutableBuffer<immutableEnsure>(data, size, size)
{
	push_back(data, size);
}

inline FixedBuffer::FixedBuffer(char* data, size_t capacity, size_t size) :
	MutableBuffer<immutableEnsure>(data, capacity, size)
{
}
// }}}
// {{{ Buffer
/**
 * \brief defines a memory buffer construction and access API.
 *
 * This class should be used when sequentially creating and reading parts from it is the main goal
 * of some certain linear information to share.
 */
class XIO_API Buffer :
	public MutableBuffer<mutableEnsure>
{
public:
	enum { CHUNK_SIZE = 4096 };

public:
	Buffer();
	explicit Buffer(size_t capacity);
	explicit Buffer(const char* value);
	explicit Buffer(const BufferRef& v);
	template<typename PodType, size_t N> explicit Buffer(PodType (&value)[N]);
	Buffer(const std::string& v);
	Buffer(const Buffer& v);
	Buffer(const BufferRef& v, size_t offset, size_t size);
	Buffer(const value_type *value, size_t size);
	Buffer(Buffer&& v);
	~Buffer();

	Buffer& operator=(Buffer&& v);
	Buffer& operator=(const Buffer& v);
	Buffer& operator=(const BufferRef& v);
	Buffer& operator=(const std::string& v);
	Buffer& operator=(const value_type* v);

	BufferSlice slice(size_t offset = 0) const;
	BufferSlice slice(size_t offset, size_t size) const;

	const char* c_str() const;

	bool setCapacity(size_t value);

	operator bool () const;
	bool operator!() const;

//	static Buffer fromCopy(const value_type *data, size_t count);
};
// }}}
// {{{ BufferSlice
/** Holds a reference to a slice (region) of a managed mutable buffer.
 */
class XIO_API BufferSlice :
	public BufferBase<Buffer>
{
public:
	BufferSlice();
	BufferSlice(Buffer& buffer, size_t offset, size_t _size);
	BufferSlice(const BufferSlice& v);

	BufferSlice& operator=(const BufferSlice& v);

	void shl(ssize_t offset = 1);
	void shr(ssize_t offset = 1);

	// random access
	reference_type operator[](size_t offset) { return data()[offset]; }
	const reference_type operator[](size_t offset) const { return data()[offset]; }

	Buffer* buffer() const { return data_.buffer(); }
};
// }}}
// {{{ free functions API

template<bool (*ensure)(void*, size_t)> XIO_API MutableBuffer<ensure>& operator<<(MutableBuffer<ensure>& b, typename MutableBuffer<ensure>::value_type v);
template<bool (*ensure)(void*, size_t)> XIO_API MutableBuffer<ensure>& operator<<(MutableBuffer<ensure>& b, int v);
template<bool (*ensure)(void*, size_t)> XIO_API MutableBuffer<ensure>& operator<<(MutableBuffer<ensure>& b, long v);
template<bool (*ensure)(void*, size_t)> XIO_API MutableBuffer<ensure>& operator<<(MutableBuffer<ensure>& b, long long v);
template<bool (*ensure)(void*, size_t)> XIO_API MutableBuffer<ensure>& operator<<(MutableBuffer<ensure>& b, unsigned v);
template<bool (*ensure)(void*, size_t)> XIO_API MutableBuffer<ensure>& operator<<(MutableBuffer<ensure>& b, unsigned long v);
template<bool (*ensure)(void*, size_t)> XIO_API MutableBuffer<ensure>& operator<<(MutableBuffer<ensure>& b, unsigned long long v);
template<bool (*ensure)(void*, size_t)> XIO_API MutableBuffer<ensure>& operator<<(MutableBuffer<ensure>& b, const MutableBuffer<ensure>& v);
template<bool (*ensure)(void*, size_t)> XIO_API MutableBuffer<ensure>& operator<<(MutableBuffer<ensure>& b, const BufferRef& v);
template<bool (*ensure)(void*, size_t)> XIO_API MutableBuffer<ensure>& operator<<(MutableBuffer<ensure>& b, const std::string& v);
template<bool (*ensure)(void*, size_t), typename PodType, size_t N> XIO_API MutableBuffer<ensure>& operator<<(MutableBuffer<ensure>& b, PodType (&v)[N]);
template<bool (*ensure)(void*, size_t)> XIO_API MutableBuffer<ensure>& operator<<(MutableBuffer<ensure>& b, const typename MutableBuffer<ensure>::value_type *v);
// }}}

//@}

// {{{ BufferTraits helper impl
inline BufferOffset::operator char* ()
{
	assert(buffer_ != nullptr && "BufferSlice must not be empty when accessing data.");
	return buffer_->data() + offset_;
}

inline BufferOffset::operator const char* () const
{
	assert(buffer_ != nullptr && "BufferSlice must not be empty when accessing data.");
	return const_cast<BufferOffset*>(this)->buffer_->data() + offset_;
}
// }}}
// {{{ BufferBase<T> impl
template<typename T>
inline bool BufferBase<T>::contains(const BufferBase<T>& ref) const
{
	return ref.cbegin() >= cbegin()
		&& ref.cend() <= cend();
}

template<typename T>
inline size_t BufferBase<T>::find(const value_type *value, size_t offset) const
{
	const char *i = cbegin() + offset;
	const char *e = cend();
	const int value_length = strlen(value);

	while (i != e) {
		if (*i == *value) {
			const char *p = i + 1;
			const char *q = value + 1;
			const char *qe = i + value_length;

			while (*p == *q && p != qe) {
				++p;
				++q;
			}

			if (p == qe) {
				return i - data();
			}
		}
		++i;
	}

	return npos;
}

template<typename T>
inline size_t BufferBase<T>::find(value_type value, size_t offset) const
{
	if (const char *p = (const char *)memchr((const void *)(data() + offset), value, size() - offset))
		return p - data();

	return npos;
}

template<typename T>
template<typename PodType, size_t N>
inline size_t BufferBase<T>::find(PodType (&value)[N], size_t offset) const
{
	const char *i = data() + offset;
	const char *e = i + size() - offset;

	while (i != e) {
		if (*i == *value) {
			const char *p = i + 1;
			const char *q = value + 1;
			const char *qe = i + N - 1;

			while (*p == *q && p != qe) {
				++p;
				++q;
			}

			if (p == qe) {
				return i - data();
			}
		}
		++i;
	}

	return npos;
}

template<typename T>
inline size_t BufferBase<T>::rfind(const value_type *value) const
{
	//! \todo implementation
	assert(0 && "not implemented");
	return npos;
}

template<typename T>
inline size_t BufferBase<T>::rfind(value_type value) const
{
	return rfind(value, size() - 1);
}

template<typename T>
inline size_t BufferBase<T>::rfind(value_type value, size_t offset) const
{
	if (empty())
		return npos;

	const char *p = data();
	const char *q = p + offset;

	for (;;) {
		if (*q == value)
			return q - p;

		if (p == q)
			break;

		--q;
	}

	return npos;
}

template<typename T>
template<typename PodType, size_t N>
size_t BufferBase<T>::rfind(PodType (&value)[N]) const
{
	if (empty())
		return npos;

	if (size() < N - 1)
		return npos;

	const char *i = end();
	const char *e = begin() + (N - 1);

	while (i != e) {
		if (*i == value[N - 1]) {
			bool found = true;

			for (int n = 0; n < N - 2; ++n) {
				if (i[n] != value[n]) {
					found = !found;
					break;
				}
			}

			if (found) {
				return i - begin();
			}
		}
		--i;
	}

	return npos;
}

template<typename T>
std::pair<BufferRef, BufferRef> BufferBase<T>::split(char delimiter) const
{
	size_t i = find(delimiter);
	if (i == npos)
		return std::make_pair(ref(), BufferRef());
	else
		return std::make_pair(ref(0, i), ref(i + 1, npos));
}

template<typename T>
std::pair<BufferRef, BufferRef> BufferBase<T>::split(const value_type* delimiter) const
{
	size_t i = find(delimiter);
	if (i == npos)
		return std::make_pair(ref(), BufferRef());
	else
		return std::make_pair(ref(0, i), ref(i + strlen(delimiter), npos));
}

template<typename T>
inline bool BufferBase<T>::begins(const BufferRef& value) const
{
	return value.size() <= size() && std::memcmp(data(), value.data(), value.size()) == 0;
}

template<typename T>
inline bool BufferBase<T>::begins(const value_type *value) const
{
	if (!value)
		return true;

	size_t len = std::strlen(value);
	return len <= size() && std::memcmp(data(), value, len) == 0;
}

template<typename T>
inline bool BufferBase<T>::begins(const std::string& value) const
{
	if (value.empty())
		return true;

	return value.size() <= size() && std::memcmp(data(), value.data(), value.size()) == 0;
}

template<typename T>
inline bool BufferBase<T>::begins(value_type value) const
{
	return size() >= 1 && data()[0] == value;
}

template<typename T>
inline bool BufferBase<T>::ends(const BufferRef& value) const
{
	if (value.empty())
		return true;

	size_t valueLength = value.size();

	if (size() < valueLength)
		return false;

	return std::memcmp(data() + size() - valueLength, value.data(), valueLength) == 0;
}

template<typename T>
inline bool BufferBase<T>::ends(value_type value) const
{
	return size() >= 1 && data()[size() - 1] == value;
}

template<typename T>
inline bool BufferBase<T>::ends(const value_type *value) const
{
	if (!value)
		return true;

	size_t valueLength = std::strlen(value);

	if (size() < valueLength)
		return false;

	return std::memcmp(data() + size() - valueLength, value, valueLength) == 0;
}

template<>
inline BufferRef BufferBase<char*>::ref(size_t offset) const
{
	assert(offset <= size());
	return BufferRef(data() + offset, size() - offset);
}

template<>
inline BufferRef BufferBase<char*>::ref(size_t offset, size_t count) const
{
	assert(offset <= size());
	assert(count == npos || offset + count <= size());

	return count != npos
		? BufferRef(data() + offset, count)
		: BufferRef(data() + offset, size() - offset);
}

template<>
inline BufferRef BufferBase<Buffer>::ref(size_t offset) const
{
	assert(offset <= size());

	return BufferRef(data() + offset, size() - offset);
}

template<>
inline BufferRef BufferBase<Buffer>::ref(size_t offset, size_t count) const
{
	assert(offset <= size());
	assert(count == npos || offset + count <= size());

	return count != npos
		? BufferRef(data() + offset, count)
		: BufferRef(data() + offset, size() - offset);
}

template<typename T>
inline BufferRef BufferBase<T>::chomp() const
{
	return ends('\n')
		? ref(0, size_ - 1)
		: ref(0, size_);
}

template<typename T>
inline BufferRef BufferBase<T>::trim() const
{
	std::size_t left = 0;
	while (left < size() && std::isspace(at(left)))
		++left;

	std::size_t right = size() - 1;
	while (right > 0 && std::isspace(at(right)))
		--right;

	return ref(left, 1 + right - left);
}

template<typename T>
inline std::string BufferBase<T>::str() const
{
	return substr(0);
}

template<typename T>
inline std::string BufferBase<T>::substr(size_t offset) const
{
	assert(offset <= size());
	ssize_t count = size() - offset;
	return count
		? std::string(data() + offset, count)
		: std::string();
}

template<typename T>
inline std::string BufferBase<T>::substr(size_t offset, size_t count) const
{
	assert(offset + count <= size());
	return count
		? std::string(data() + offset, count)
		: std::string();
}

template<typename T>
template<typename U>
inline U BufferBase<T>::hex() const
{
	auto i = begin();
	auto e = end();

	// empty string
	if (i == e)
		return 0;

	U val = 0;
	while (i != e) {
		if (!std::isxdigit(*i))
			break;

		if (val)
			val *= 16;

		if (std::isdigit(*i))
			val += *i++ - '0';
		else if (*i >= 'a' && *i <= 'f')
			val += 10 + *i++ - 'a';
		else // 'A' .. 'F'
			val += 10 + *i++ - 'A';
	}

	return val;
}

template<typename T>
inline bool BufferBase<T>::toBool() const
{
	if (iequals(*this, "true"))
		return true;

	if (equals(*this, "1"))
		return true;

	return false;
}

template<typename T>
inline int BufferBase<T>::toInt() const
{
	auto i = cbegin();
	auto e = cend();

	// empty string
	if (i == e)
		return 0;

	// parse sign
	bool sign = false;
	if (*i == '-') {
		sign = true;
		++i;

		if (i == e) {
			return 0;
		}
	} else if (*i == '+') {
		++i;

		if (i == e)
			return 0;
	}

	// parse digits
	int val = 0;
	while (i != e) {
		if (*i < '0' || *i > '9')
			break;

		if (val)
			val *= 10;

		val += *i++ - '0';
	}

	// parsing succeed.
	if (sign)
		val = -val;

	return val;
}

template<typename T>
inline double BufferBase<T>::toDouble() const
{
	char* endptr = nullptr;
	double result = strtod(cbegin(), &endptr);
	if (endptr <= cend())
		return result;

	return 0.0;
}

template<typename T>
inline float BufferBase<T>::toFloat() const
{
	char* tmp = (char*) alloca(size() + 1);
	std::memcpy(tmp, cbegin(), size());
	tmp[size()] = '\0';

	return strtof(tmp, nullptr);
}

template<typename T>
inline void BufferBase<T>::dump(const char *description) const
{
	BufferRef::dump(data(), size(), description);
}

// --------------------------------------------------------------------------
template<typename T>
inline bool equals(const BufferBase<T>& a, const BufferBase<T>& b)
{
	if (&a == &b)
		return true;

	if (a.size() != b.size())
		return false;

	return std::memcmp(a.data(), b.data(), a.size()) == 0;
}

template<typename T, typename PodType, std::size_t N>
bool equals(const BufferBase<T>& a, PodType (&b)[N])
{
	const std::size_t bsize = N - 1;

	if (a.size() != bsize)
		return false;

	return std::memcmp(a.data(), b, bsize) == 0;
}

template<typename T>
inline bool equals(const BufferBase<T>& a, const std::string& b)
{
	if (a.size() != b.size())
		return false;

	return std::memcmp(a.data(), b.data(), b.size()) == 0;
}

inline bool equals(const std::string& a, const std::string& b)
{
	if (a.size() != b.size())
		return false;

	return std::memcmp(a.data(), b.data(), b.size()) == 0;
}

// --------------------------------------------------------------------

template<typename T>
inline bool iequals(const BufferBase<T>& a, const BufferBase<T>& b)
{
	if (&a == &b)
		return true;

	if (a.size() != b.size())
		return false;

	return strncasecmp(a.data(), b.data(), a.size()) == 0;
}

template<typename T, typename PodType, std::size_t N>
bool iequals(const BufferBase<T>& a, PodType (&b)[N])
{
	const std::size_t bsize = N - 1;

	if (a.size() != bsize)
		return false;

	return strncasecmp(a.data(), b, bsize) == 0;
}

template<typename T>
inline bool iequals(const BufferBase<T>& a, const std::string& b)
{
	if (a.size() != b.size())
		return false;

	return strncasecmp(a.data(), b.data(), b.size()) == 0;
}

inline bool iequals(const std::string& a, const std::string& b)
{
	if (a.size() != b.size())
		return false;

	return strncasecmp(a.data(), b.data(), b.size()) == 0;
}

// ------------------------------------------------------------------------
template<typename T>
inline bool operator==(const BufferBase<T>& a, const BufferBase<T>& b)
{
	return equals(a, b);
}

template<typename T>
inline bool operator==(const BufferBase<T>& a, const std::string& b)
{
	return equals(a, b);
}

template<typename T, typename PodType, std::size_t N>
inline bool operator==(const BufferBase<T>& a, PodType (&b)[N])
{
	return equals<T, PodType, N>(a, b);
}

template<typename T, typename PodType, std::size_t N>
inline bool operator==(PodType (&a)[N], const BufferBase<T>& b)
{
	return equals<T, PodType, N>(b, a);
}

inline bool immutableEnsure(void* self, size_t size)
{
	MutableBuffer<immutableEnsure>* buffer = (MutableBuffer<immutableEnsure>*) self;
	return size <= buffer->capacity();
}

inline bool mutableEnsure(void* self, size_t size)
{
	Buffer* buffer = (Buffer*) self;
	return size > buffer->capacity() || size == 0 
		? buffer->setCapacity(size)
		: true;
}
// }}}
// {{{ BufferRef impl
inline BufferRef& BufferRef::operator=(const BufferRef& v) {
	data_ = v.data_;
	size_ = v.size_;
	return *this;
}
// }}}
// {{{ MutableBuffer<ensure> impl
template<bool (*ensure)(void*, size_t)>
inline MutableBuffer<ensure>::MutableBuffer() :
	BufferRef(),
	capacity_(0)
{
}

template<bool (*ensure)(void*, size_t)>
inline MutableBuffer<ensure>::MutableBuffer(MutableBuffer&& v) :
	BufferRef(std::move(v)),
	capacity_(std::move(v.capacity_))
{
}

template<bool (*ensure)(void*, size_t)>
inline MutableBuffer<ensure>::MutableBuffer(char* value, size_t size) :
	BufferRef(value, size),
	capacity_(size)
{
}

template<bool (*ensure)(void*, size_t)>
inline MutableBuffer<ensure>::MutableBuffer(char* value, size_t capacity, size_t size) :
	BufferRef(value, size),
	capacity_(capacity)
{
}

template<bool (*ensure)(void*, size_t)>
inline void MutableBuffer<ensure>::swap(MutableBuffer<ensure>& other)
{
	std::swap(data_, other.data_);
	std::swap(size_, other.size_);
	std::swap(capacity_, other.capacity_);
}

template<bool (*ensure)(void*, size_t)>
inline bool MutableBuffer<ensure>::resize(size_t value)
{
	if (value > capacity_)
		return false;

	size_ = value;
	return true;
}

template<bool (*ensure)(void*, size_t)>
inline size_t MutableBuffer<ensure>::capacity() const
{
	return capacity_;
}

template<bool (*ensure)(void*, size_t)>
inline bool MutableBuffer<ensure>::reserve(size_t value)
{
	return ensure(this, value);
}

// TODO: implement operator bool() and verify it's working for: 
//     if (BufferRef r = foo()) {}
//     if (foo()) {}
//     if (someBufferRef) {}

template<bool (*ensure)(void*, size_t)>
inline bool MutableBuffer<ensure>::operator!() const
{
	return empty();
}

template<bool (*ensure)(void*, size_t)>
inline void MutableBuffer<ensure>::push_back(value_type value)
{
	if (reserve(size() + sizeof(value))) {
		data_[size_++] = value;
	}
}

template<bool (*ensure)(void*, size_t)>
inline void MutableBuffer<ensure>::push_back(int value)
{
	char buf[32];
	int n = std::snprintf(buf, sizeof(buf), "%d", value);
	push_back(buf, n);
}

template<bool (*ensure)(void*, size_t)>
inline void MutableBuffer<ensure>::push_back(long value)
{
	char buf[32];
	int n = std::snprintf(buf, sizeof(buf), "%ld", value);
	push_back(buf, n);
}

template<bool (*ensure)(void*, size_t)>
inline void MutableBuffer<ensure>::push_back(long long value)
{
	char buf[32];
	int n = std::snprintf(buf, sizeof(buf), "%lld", value);
	push_back(buf, n);
}

template<bool (*ensure)(void*, size_t)>
inline void MutableBuffer<ensure>::push_back(unsigned value)
{
	char buf[32];
	int n = std::snprintf(buf, sizeof(buf), "%u", value);
	push_back(buf, n);
}

template<bool (*ensure)(void*, size_t)>
inline void MutableBuffer<ensure>::push_back(unsigned long value)
{
	char buf[32];
	int n = std::snprintf(buf, sizeof(buf), "%lu", value);
	push_back(buf, n);
}

template<bool (*ensure)(void*, size_t)>
inline void MutableBuffer<ensure>::push_back(unsigned long long value)
{
	char buf[32];
	int n = std::snprintf(buf, sizeof(buf), "%llu", value);
	push_back(buf, n);
}

template<bool (*ensure)(void*, size_t)>
inline void MutableBuffer<ensure>::push_back(const BufferRef& value)
{
	if (size_t len = value.size()) {
		if (reserve(size_ + len)) {
			std::memcpy(end(), value.cbegin(), len);
			size_ += len;
		}
	}
}

template<bool (*ensure)(void*, size_t)>
inline void MutableBuffer<ensure>::push_back(const BufferRef& value, size_t offset, size_t length)
{
	assert(value.size() <= offset + length);

	if (!length)
		return;

	if (reserve(size_ + length)) {
		memcpy(end(), value.cbegin() + offset, length);
		size_ += length;
	}
}

template<bool (*ensure)(void*, size_t)>
inline void MutableBuffer<ensure>::push_back(const std::string& value)
{
	if (size_t len = value.size()) {
		if (reserve(size_ + len)) {
			std::memcpy(end(), value.data(), len);
			size_ += len;
		}
	}
}

template<bool (*ensure)(void*, size_t)>
template<typename PodType, size_t N>
inline void MutableBuffer<ensure>::push_back(PodType (&value)[N])
{
	push_back(reinterpret_cast<const void *>(value), N - 1);
}

template<bool (*ensure)(void*, size_t)>
inline void MutableBuffer<ensure>::push_back(const void* value, size_t size)
{
	if (size) {
		if (reserve(size_ + size)) {
			std::memcpy(end(), value, size);
			size_ += size;
		}
	}
}

template<bool (*ensure)(void*, size_t)>
inline MutableBuffer<ensure>& MutableBuffer<ensure>::vprintf(const char* fmt, va_list args)
{
	reserve(size() + strlen(fmt) + 1);

	while (true) {
		va_list va;
		va_copy(va, args);
		ssize_t buflen = vsnprintf(data_ + size_, capacity_ - size_, fmt, va);
		va_end(va);

		if (buflen >= -1 && buflen < static_cast<ssize_t>(capacity_ - size_)) {
			resize(size_ + buflen);
			break; // success
		}

		buflen = buflen > -1
			? buflen + 1      // glibc >= 2.1
			: capacity_ * 2;  // glibc <= 2.0

		if (!setCapacity(capacity_ + buflen)) {
			// increasing capacity failed
			data_[capacity_ - 1] = '\0';
			break; // alloc failure
		}
	}

	return *this;
}

template<bool (*ensure)(void*, size_t)>
inline MutableBuffer<ensure>& MutableBuffer<ensure>::printf(const char* fmt)
{
	push_back(fmt);
	return *this;
}

template<bool (*ensure)(void*, size_t)>
template<typename... Args>
inline MutableBuffer<ensure>& MutableBuffer<ensure>::printf(const char* fmt, Args... args)
{
	reserve(size() + strlen(fmt) + 1);

	while (true) {
		ssize_t buflen = snprintf(data_ + size_, capacity_ - size_, fmt, args...);

		if (buflen >= -1 && buflen < static_cast<ssize_t>(capacity_ - size_)) {
			resize(size_ + buflen);
			break; // success
		}

		buflen = buflen > -1
			? buflen + 1      // glibc >= 2.1
			: capacity_ * 2;  // glibc <= 2.0

		if (!reserve(capacity_ + buflen)) {
			// increasing capacity failed
			data_[capacity_ - 1] = '\0';
			break; // alloc failure
		}
	}

	return *this;
}

template<bool (*ensure)(void*, size_t)>
inline typename MutableBuffer<ensure>::reference_type MutableBuffer<ensure>::operator[](size_t index)
{
	assert(index < size_);

	return data_[index];
}

template<bool (*ensure)(void*, size_t)>
inline const typename MutableBuffer<ensure>::reference_type MutableBuffer<ensure>::operator[](size_t index) const
{
	assert(index < size_);

	return data_[index];
}

/*
template<bool (*ensure)(void*, size_t)>
inline MutableBuffer MutableBuffer<ensure>::fromCopy(const value_type *data, size_t count)
{
	MutableBuffer result(count);
	result.push_back(data, count);
	return result;
}
*/
// }}}
// {{{ MutableBuffer<ensure>& operator<<() impl
template<bool (*ensure)(void*, size_t)>
inline MutableBuffer<ensure>& operator<<(MutableBuffer<ensure>& b, typename MutableBuffer<ensure>::value_type v)
{
	b.push_back(v);
	return b;
}

template<bool (*ensure)(void*, size_t)>
inline MutableBuffer<ensure>& operator<<(MutableBuffer<ensure>& b, int v)
{
	b.push_back(v);
	return b;
}

template<bool (*ensure)(void*, size_t)>
inline MutableBuffer<ensure>& operator<<(MutableBuffer<ensure>& b, long v)
{
	b.push_back(v);
	return b;
}

template<bool (*ensure)(void*, size_t)>
inline MutableBuffer<ensure>& operator<<(MutableBuffer<ensure>& b, long long v)
{
	b.push_back(v);
	return b;
}

template<bool (*ensure)(void*, size_t)>
inline MutableBuffer<ensure>& operator<<(MutableBuffer<ensure>& b, unsigned v)
{
	b.push_back(v);
	return b;
}

template<bool (*ensure)(void*, size_t)>
inline MutableBuffer<ensure>& operator<<(MutableBuffer<ensure>& b, unsigned long v)
{
	b.push_back(v);
	return b;
}

template<bool (*ensure)(void*, size_t)>
inline MutableBuffer<ensure>& operator<<(MutableBuffer<ensure>& b, unsigned long long v)
{
	b.push_back(v);
	return b;
}

template<bool (*ensure)(void*, size_t)>
inline MutableBuffer<ensure>& operator<<(MutableBuffer<ensure>& b, const MutableBuffer<ensure>& v)
{
	b.push_back(v);
	return b;
}

template<bool (*ensure)(void*, size_t)>
inline MutableBuffer<ensure>& operator<<(MutableBuffer<ensure>& b, const BufferRef& v)
{
	b.push_back(v);
	return b;
}

template<bool (*ensure)(void*, size_t)>
inline MutableBuffer<ensure>& operator<<(MutableBuffer<ensure>& b, const std::string& v)
{
	b.push_back(v);
	return b;
}

template<bool (*ensure)(void*, size_t), typename PodType, size_t N>
inline MutableBuffer<ensure>& operator<<(MutableBuffer<ensure>& b, PodType (&v)[N])
{
	b.template push_back<PodType, N>(v);
	return b;
}

template<bool (*ensure)(void*, size_t)>
inline MutableBuffer<ensure>& operator<<(MutableBuffer<ensure>& b, const typename MutableBuffer<ensure>::value_type *v)
{
	b.push_back(v);
	return b;
}
// }}}
// {{{ Buffer impl
inline Buffer::Buffer() :
	MutableBuffer<mutableEnsure>()
{
}

inline Buffer::Buffer(const BufferRef& v, size_t offset, size_t count) :

	MutableBuffer<mutableEnsure>()
{
	assert(offset + count <= v.size());

	push_back(v.data() + offset, count);
}

inline Buffer::Buffer(size_t _capacity) :
	MutableBuffer<mutableEnsure>()
{
	reserve(_capacity);
}

inline Buffer::Buffer(const char* v) :
	MutableBuffer<mutableEnsure>()
{
	push_back(v);
}

inline Buffer::Buffer(const std::string& v) :
	MutableBuffer<mutableEnsure>()
{
	push_back(v.c_str(), v.size() + 1);
	resize(v.size());
}

inline Buffer::Buffer(const Buffer& v) :
	MutableBuffer<mutableEnsure>()
{
	push_back(v.data(), v.size());
}

inline Buffer::~Buffer()
{
	reserve(0);
}

inline Buffer& Buffer::operator=(Buffer&& v)
{
	reserve(0); // special case, frees the buffer if available and managed

	data_ = v.data_;
	size_ = v.size_;
	capacity_ = v.capacity_;

	v.data_ = 0;
	v.size_ = 0;
	v.capacity_ = 0;

	return *this;
}

inline Buffer& Buffer::operator=(const BufferRef& v)
{
	clear();
	push_back(v);

	return *this;
}

inline Buffer& Buffer::operator=(const std::string& v)
{
	clear();
	push_back(v.data(), v.size());

	return *this;
}

inline Buffer& Buffer::operator=(const value_type* v)
{
	clear();
	push_back(v, std::strlen(v));

	return *this;
}

inline BufferSlice Buffer::slice(size_t offset) const
{
	assert(offset <= size());
	return BufferSlice(*(Buffer*) this, offset, size() - offset);
}

inline BufferSlice Buffer::slice(size_t offset, size_t count) const
{
	assert(offset <= size());
	assert(count == npos || offset + count <= size());

	return count != npos
		? BufferSlice(*(Buffer*) this, offset, count)
		: BufferSlice(*(Buffer*) this, offset, size() - offset);
}

inline const Buffer::value_type *Buffer::c_str() const
{
	if (const_cast<Buffer*>(this)->reserve(size_ + 1))
		const_cast<Buffer*>(this)->data_[size_] = '\0';

	return data_;
}

// }}}
// {{{ BufferSlice impl
inline BufferSlice::BufferSlice() :
	BufferBase<Buffer>()
{
}

inline BufferSlice::BufferSlice(Buffer& buffer, size_t offset, size_t size) :
	BufferBase<Buffer>(data_type(&buffer, offset), size)
{
}

inline BufferSlice::BufferSlice(const BufferSlice& v) :
	BufferBase<Buffer>(v)
{
}

inline BufferSlice& BufferSlice::operator=(const BufferSlice& v)
{
	data_ = v.data_;
	size_ = v.size_;

	return *this;
}

/** shifts view's left margin by given bytes to the left, thus, increasing view's size.
 */
inline void BufferSlice::shl(ssize_t value)
{
	assert(data_.offset_ - value >= 0);
	data_.offset_ -= value;
}

/** shifts view's right margin by given bytes to the right, thus, increasing view's size.
 */
inline void BufferSlice::shr(ssize_t value)
{
	size_ += value;
}
// }}}

} // namespace xio

// {{{ std::hash<BufferBase<T>>
namespace xio {
	// Fowler / Noll / Vo (FNV) Hash-Implementation
	template<typename T>
	uint32_t hash(const T& array) noexcept {
		uint32_t result = 2166136261u;

		for (auto value: array) {
			result ^= value;
			result *= 16777619;
		}

		return result;
	}
}

namespace std {
	template<>
	struct hash<xio::BufferSlice> {
		typedef xio::BufferSlice argument_type;
		typedef uint32_t result_type;

		result_type operator()(const argument_type& value) const noexcept {
			return xio::hash(value);
		}
	};

	template<>
	struct hash<xio::BufferRef> {
		typedef xio::BufferRef  argument_type;
		typedef uint32_t result_type;

		result_type operator()(const argument_type& value) const noexcept {
			return xio::hash(value);
		}
	};

	template<>
	struct hash<xio::Buffer> {
		typedef xio::Buffer argument_type;
		typedef uint32_t result_type;

		result_type operator()(const argument_type& value) const noexcept {
			return xio::hash(value);
		}
	};
}
// }}}
