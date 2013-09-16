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

namespace xio {

//! \addtogroup base
//@{

class Buffer;
class BufferRef;
class BufferView;

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

/**
 * \brief helper class for BufferRef to point inside a Buffer.
 */
struct BufferOffset {
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
};

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

	static const size_t npos = size_t(-1);

protected:
	data_type data_;
	size_t size_;

public:
	BufferBase() : data_(), size_(0) {}
	BufferBase(data_type data, size_t size) : data_(data), size_(size) {}
	BufferBase(const BufferBase<T>& v) : data_(v.data_), size_(v.size_) {}
	BufferBase(BufferBase<T>&& v) :
		data_(std::move(v.data_)),
		size_(std::move(v.size_))
	{
	}

	BufferBase<T>& operator=(BufferBase<T>& v) {
		data_ = v.data_;
		size_ = v.size_;
		return *this;
	}

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
	BufferView view(size_t offset = 0) const;
	BufferView view(size_t offset, size_t size) const;
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
// {{{ BufferRef API
class XIO_API BufferRef :
	public BufferBase<char*>
{
public:
	BufferRef();
	BufferRef(const char* data, size_t size);
	template<typename PodType, size_t N> explicit BufferRef(PodType (&value)[N]);

	explicit BufferRef(const BufferRef& v);
	BufferRef(BufferRef&& v);

	BufferRef& operator=(const BufferRef& v);

	static void dump(const void *bytes, std::size_t length, const char *description);
};
// }}}
// {{{ Buffer API
/**
 * \brief defines a memory buffer construction and access API.
 *
 * This class should be used when sequentially creating and reading parts from it is the main goal
 * of some certain linear information to share.
 */
class XIO_API Buffer :
	public BufferRef
{
public:
	//static const size_t CHUNK_SIZE = 4096;
	enum { CHUNK_SIZE = 4096 };

	struct helper { int i; };
	typedef int (helper::*helper_type);

protected:
	size_t capacity_;

public:
	Buffer();
	explicit Buffer(size_t _capacity);

	Buffer(const char* value);
	Buffer(const std::string& v);
	template<typename PodType, size_t N> explicit Buffer(PodType (&value)[N]);
	Buffer(const Buffer& v);
	Buffer(Buffer&& v);
	explicit Buffer(const BufferRef& v);
	Buffer(const BufferRef& v, size_t offset, size_t size);
	Buffer(const value_type *_data, size_t _size); // XXX better be private?

	Buffer& operator=(Buffer&& v);
	Buffer& operator=(const Buffer& v);
	Buffer& operator=(const BufferRef& v);
	Buffer& operator=(const std::string& v);
	Buffer& operator=(const value_type* v);

	~Buffer();

	void swap(Buffer& other);

	bool resize(size_t value);

	size_t capacity() const;
	virtual bool setCapacity(size_t value);

	bool reserve(size_t value);

	operator helper_type() const;
	bool operator!() const;

	// buffer builders
	void push_back(value_type value);
	void push_back(int value);
	void push_back(long value);
	void push_back(long long value);
	void push_back(unsigned value);
	void push_back(unsigned long value);
	void push_back(unsigned long long value);
	void push_back(const value_type *value);
	void push_back(const Buffer& value);
	void push_back(const BufferRef& value);
	void push_back(const BufferRef& value, size_t offset, size_t length);
	void push_back(const std::string& value);
	void push_back(const void *value, size_t size);
	template<typename PodType, size_t N> void push_back(PodType (&value)[N]);

	template<typename... Args>
	Buffer& printf(const char* fmt, Args... args);

	// random access
	reference_type operator[](size_t index);
	const reference_type operator[](size_t index) const;

	const value_type *c_str() const;

	static Buffer fromCopy(const value_type *data, size_t count);

	bool contains(const BufferRef& ref) const;
};
// }}}
// {{{ BufferView API
/** holds a reference to a region of a buffer
 */
class XIO_API BufferView :
	public BufferBase<Buffer>
{
public:
	BufferView();
	BufferView(Buffer& buffer, size_t offset, size_t _size);
	BufferView(const BufferView& v);

	BufferView& operator=(const BufferView& v);

	void shl(ssize_t offset = 1);
	void shr(ssize_t offset = 1);

	// random access
	reference_type operator[](size_t offset) { return data()[offset]; }
	const reference_type operator[](size_t offset) const { return data()[offset]; }

	Buffer* buffer() const { return data_.buffer(); }
};
// }}}
// {{{ free functions API
XIO_API Buffer& operator<<(Buffer& b, Buffer::value_type v);
XIO_API Buffer& operator<<(Buffer& b, int v);
XIO_API Buffer& operator<<(Buffer& b, long v);
XIO_API Buffer& operator<<(Buffer& b, long long v);
XIO_API Buffer& operator<<(Buffer& b, unsigned v);
XIO_API Buffer& operator<<(Buffer& b, unsigned long v);
XIO_API Buffer& operator<<(Buffer& b, unsigned long long v);
XIO_API Buffer& operator<<(Buffer& b, const Buffer::value_type *v);
XIO_API Buffer& operator<<(Buffer& b, const Buffer& v);
XIO_API Buffer& operator<<(Buffer& b, const BufferRef& v);
XIO_API Buffer& operator<<(Buffer& b, const std::string& v);
template<typename PodType, size_t N> XIO_API Buffer& operator<<(Buffer& b, PodType (&v)[N]);
// }}}

} // namespace xio

//@}

// {{{ BufferTraits helper impl
namespace xio {

inline BufferOffset::operator char* ()
{
	assert(buffer_ != nullptr && "BufferView must not be empty when accessing data.");
	return buffer_->data() + offset_;
}

inline BufferOffset::operator const char* () const
{
	assert(buffer_ != nullptr && "BufferView must not be empty when accessing data.");
	return const_cast<BufferOffset*>(this)->buffer_->data() + offset_;
}

} // namespace xio
// }}}
// {{{ BufferBase<T> impl
namespace xio {

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
	return value.size() <= size() && memcmp(data(), value.data(), value.size()) == 0;
}

template<typename T>
inline bool BufferBase<T>::begins(const value_type *value) const
{
	if (!value)
		return true;

	size_t len = std::strlen(value);
	return len <= size() && memcmp(data(), value, len) == 0;
}

template<typename T>
inline bool BufferBase<T>::begins(const std::string& value) const
{
	if (value.empty())
		return true;

	return value.size() <= size() && memcmp(data(), value.data(), value.size()) == 0;
}

template<typename T>
inline bool BufferBase<T>::begins(value_type value) const
{
	return size() >= 1 && data()[0] == value;
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

	return memcmp(data() + size() - valueLength, value, valueLength) == 0;
}


template<>
inline BufferView BufferBase<char*>::view(size_t offset) const
{
	assert(offset <= size());
	return BufferView(*(Buffer*) this, offset, size() - offset);
}

template<>
inline BufferView BufferBase<char*>::view(size_t offset, size_t count) const
{
	assert(offset <= size());
	assert(count == npos || offset + count <= size());

	return count != npos
		? BufferView(*(Buffer*) this, offset, count)
		: BufferView(*(Buffer*) this, offset, size() - offset);
}

template<>
inline BufferView BufferBase<Buffer>::view(size_t offset) const
{
	assert(offset <= size());

	return BufferView(*data_.buffer(), data_.offset() + offset, size() - offset);
}

template<>
inline BufferView BufferBase<Buffer>::view(size_t offset, size_t count) const
{
	assert(offset <= size());
	assert(count == npos || offset + count <= size());

	return count != npos
		? BufferView(*data_.buffer(), data_.offset() + offset, count)
		: BufferView(*data_.buffer(), data_.offset() + offset, size() - offset);
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
} // namespace xio
// }}}
// {{{ BufferRef impl
namespace xio {
inline BufferRef::BufferRef() :
	BufferBase<char*>()
{
}

inline BufferRef::BufferRef(const char* data, size_t size) :
	BufferBase<char*>((char*) data, size)
{
}

template<typename PodType, size_t N>
inline BufferRef::BufferRef(PodType (&value)[N]) :
	BufferBase<char*>((char*) value, N - 1)
{
}

inline BufferRef::BufferRef(const BufferRef& v) :
	BufferBase<char*>(v)
{
}

inline BufferRef::BufferRef(BufferRef&& v) :
	BufferBase<char*>(v)
{
}

inline BufferRef& BufferRef::operator=(const BufferRef& v)
{
	data_ = v.data_;
	size_ = v.size_;

	return *this;
}
} // namespace xio
// }}}
// {{{ Buffer impl
namespace xio {
// {{{ Buffer impl
inline Buffer::Buffer() :
	BufferRef(),
	capacity_(0)
{
}

inline Buffer::Buffer(size_t _capacity) :
	BufferRef(),
	capacity_(0)
{
	setCapacity(_capacity);
}

inline Buffer::Buffer(const char* v) :
	BufferRef(),
	capacity_(0)
{
	push_back(v);
}

inline Buffer::Buffer(const std::string& v) :
	BufferRef(),
	capacity_(0)
{
	push_back(v.c_str(), v.size() + 1);
	resize(v.size());
}

template<typename PodType, size_t N>
inline Buffer::Buffer(PodType (&value)[N]) :
	BufferRef(),
	capacity_(0)
{
	push_back(value, N);
	resize(N - 1);
}

inline Buffer::Buffer(const Buffer& v) :
	BufferRef(),
	capacity_(0)
{
	push_back(v.data(), v.size());
}

inline Buffer::Buffer(Buffer&& v) :
	BufferRef(std::move(v)),
	capacity_(v.capacity_)
{
	v.capacity_ = 0;
}

inline Buffer::Buffer(const value_type* _data, size_t _size) :
	BufferRef(const_cast<value_type*>(_data), _size),
	capacity_(_size)
{
}

inline Buffer::Buffer(const BufferRef& v) :
	BufferRef(),
	capacity_(0)
{
	push_back(v.data(), v.size());
}

inline Buffer::Buffer(const BufferRef& v, size_t offset, size_t count) :
	BufferRef(),
	capacity_(0)
{
	assert(offset + count <= v.size());

	push_back(v.data() + offset, count);
}

inline Buffer& Buffer::operator=(Buffer&& v)
{
	if (capacity_)
		setCapacity(0);

	data_ = v.data_;
	size_ = v.size_;
	capacity_ = v.capacity_;

	v.data_ = 0;
	v.size_ = 0;
	v.capacity_ = 0;

	return *this;
}

inline Buffer& Buffer::operator=(const Buffer& v)
{
	clear();
	push_back(v);

	return *this;
}

inline Buffer& Buffer::operator=(const BufferRef& v)
{
	clear();
	push_back(v.data(), v.size());

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

inline Buffer::~Buffer()
{
	setCapacity(0);
}

inline void Buffer::swap(Buffer& other)
{
	std::swap(data_, other.data_);
	std::swap(size_, other.size_);
	std::swap(capacity_, other.capacity_);
}

inline const Buffer::value_type *Buffer::c_str() const
{
	if (const_cast<Buffer*>(this)->reserve(size_ + 1))
		const_cast<Buffer*>(this)->data_[size_] = '\0';

	return data_;
}

inline bool Buffer::resize(size_t value)
{
	if (!reserve(value))
		return false;

	size_ = value;
	return true;
}

inline size_t Buffer::capacity() const
{
	return capacity_;
}

inline bool Buffer::reserve(size_t value)
{
	if (value <= capacity_)
		return true;

	return setCapacity(value);
}

inline Buffer::operator helper_type() const
{
	return !empty() ? &helper::i : 0;
}

inline bool Buffer::operator!() const
{
	return empty();
}

inline void Buffer::push_back(value_type value)
{
	if (reserve(size_ + sizeof(value))) {
		data_[size_++] = value;
	}
}

inline void Buffer::push_back(int value)
{
	char buf[32];
	int n = std::snprintf(buf, sizeof(buf), "%d", value);
	push_back(buf, n);
}

inline void Buffer::push_back(long value)
{
	char buf[32];
	int n = std::snprintf(buf, sizeof(buf), "%ld", value);
	push_back(buf, n);
}

inline void Buffer::push_back(long long value)
{
	char buf[32];
	int n = std::snprintf(buf, sizeof(buf), "%lld", value);
	push_back(buf, n);
}

inline void Buffer::push_back(unsigned value)
{
	char buf[32];
	int n = std::snprintf(buf, sizeof(buf), "%u", value);
	push_back(buf, n);
}

inline void Buffer::push_back(unsigned long value)
{
	char buf[32];
	int n = std::snprintf(buf, sizeof(buf), "%lu", value);
	push_back(buf, n);
}

inline void Buffer::push_back(unsigned long long value)
{
	char buf[32];
	int n = std::snprintf(buf, sizeof(buf), "%llu", value);
	push_back(buf, n);
}

inline void Buffer::push_back(const value_type *value)
{
	if (size_t len = std::strlen(value)) {
		if (reserve(size_ + len)) {
			std::memcpy(end(), value, len);
			size_ += len;
		}
	}
}

inline void Buffer::push_back(const Buffer& value)
{
	if (size_t len = value.size()) {
		if (reserve(size_ + len)) {
			std::memcpy(end(), value.cbegin(), len);
			size_ += len;
		}
	}
}

inline void Buffer::push_back(const BufferRef& value)
{
	if (size_t len = value.size()) {
		if (reserve(size_ + len)) {
			std::memcpy(end(), value.cbegin(), len);
			size_ += len;
		}
	}
}

inline void Buffer::push_back(const BufferRef& value, size_t offset, size_t length)
{
	assert(value.size() <= offset + length);

	if (!length)
		return;

	if (reserve(size_ + length)) {
		memcpy(end(), value.cbegin() + offset, length);
		size_ += length;
	}
}

inline void Buffer::push_back(const std::string& value)
{
	if (size_t len = value.size()) {
		if (reserve(size_ + len)) {
			std::memcpy(end(), value.data(), len);
			size_ += len;
		}
	}
}

inline void Buffer::push_back(const void* value, size_t size)
{
	if (size) {
		if (reserve(size_ + size)) {
			std::memcpy(end(), value, size);
			size_ += size;
		}
	}
}

template<typename PodType, size_t N>
inline void Buffer::push_back(PodType (&value)[N])
{
	push_back(reinterpret_cast<const void *>(value), N - 1);
}

template<typename... Args>
inline Buffer& Buffer::printf(const char* fmt, Args... args)
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

		if (!setCapacity(capacity_ + buflen)) {
			// increasing capacity failed
			data_[capacity_ - 1] = '\0';
			break; // alloc failure
		}
	}

	return *this;
}

inline Buffer::reference_type Buffer::operator[](size_t index)
{
	assert(index < size_);

	return data_[index];
}

inline const Buffer::reference_type Buffer::operator[](size_t index) const
{
	assert(index < size_);

	return data_[index];
}

inline Buffer Buffer::fromCopy(const value_type *data, size_t count)
{
	Buffer result(count);
	result.push_back(data, count);
	return result;
}

inline bool Buffer::contains(const BufferRef& ref) const
{
	return ref.cbegin() >= cbegin()
		&& ref.cend() <= cend();
}
// }}}
// {{{ free function impl
inline Buffer& operator<<(Buffer& b, Buffer::value_type v)
{
	b.push_back(v);
	return b;
}

inline Buffer& operator<<(Buffer& b, int v)
{
	b.push_back(v);
	return b;
}

inline Buffer& operator<<(Buffer& b, long v)
{
	b.push_back(v);
	return b;
}

inline Buffer& operator<<(Buffer& b, long long v)
{
	b.push_back(v);
	return b;
}

inline Buffer& operator<<(Buffer& b, unsigned v)
{
	b.push_back(v);
	return b;
}

inline Buffer& operator<<(Buffer& b, unsigned long v)
{
	b.push_back(v);
	return b;
}

inline Buffer& operator<<(Buffer& b, unsigned long long v)
{
	b.push_back(v);
	return b;
}

inline Buffer& operator<<(Buffer& b, const Buffer::value_type *v)
{
	b.push_back(v);
	return b;
}

inline Buffer& operator<<(Buffer& b, const Buffer& v)
{
	b.push_back(v);
	return b;
}

inline Buffer& operator<<(Buffer& b, const BufferRef& v)
{
	b.push_back(v);
	return b;
}

inline Buffer& operator<<(Buffer& b, const std::string& v)
{
	b.push_back(v);
	return b;
}

template<typename PodType, size_t N> inline Buffer& operator<<(Buffer& b, PodType (&v)[N])
{
	b.push_back<PodType, N>(v);
	return b;
}
// }}}
} // namespace xio
// }}}
// {{{ BufferView impl
namespace xio {

inline BufferView::BufferView() :
	BufferBase<Buffer>()
{
}

inline BufferView::BufferView(Buffer& buffer, size_t offset, size_t size) :
	BufferBase<Buffer>(data_type(&buffer, offset), size)
{
}

inline BufferView::BufferView(const BufferView& v) :
	BufferBase<Buffer>(v)
{
}

inline BufferView& BufferView::operator=(const BufferView& v)
{
	data_ = v.data_;
	size_ = v.size_;

	return *this;
}

/** shifts view's left margin by given bytes to the left, thus, increasing view's size.
 */
inline void BufferView::shl(ssize_t value)
{
	data_.offset_ -= value;
}

/** shifts view's right margin by given bytes to the right, thus, increasing view's size.
 */
inline void BufferView::shr(ssize_t value)
{
	size_ += value;
}

} // namespace xio
// }}}
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
	struct hash<xio::BufferView> {
		typedef xio::BufferView argument_type;
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
