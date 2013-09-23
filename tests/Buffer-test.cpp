/* <src/x0d.cpp>
 *
 * This file is part of the x0 web server project and is released under GPL-3.
 * http://www.xzero.io/
 *
 * (c) 2009-2013 Christian Parpart <trapni@gmail.com>
 */

#include <gtest/gtest.h>
#include <xio/Buffer.h>

using namespace xio;

// {{{ BufferBase<>
// XXX we're testing BufferBase members via bufferRef
// - begin, end, cbegin, cend
// - find, rfind
// - contains
// - split
// - begins, ends, ibegins, iends
// - view, ref
// - chomp, trim
// - str, substr
// - hex<>()
// - toBool, toInt, toDouble, toFloat
TEST(BufferBase, find1)
{
	BufferRef b("fnorded,by,nature");
	ASSERT_EQ(0, b.find("fnord"));
	ASSERT_EQ(2, b.find("ord"));
	ASSERT_EQ(BufferRef::npos, b.find("not-found"));
}

TEST(BufferBase, split1)
{
	Buffer b("fnorded,by,nature");
	auto s = b.split(',');
	ASSERT_EQ("fnorded", s.first);
	ASSERT_EQ("by,nature", s.second);
}

TEST(BufferBase, split2)
{
	BufferRef b("fnorded//by//nature");
	auto s = b.split("//");
	ASSERT_EQ("fnorded", s.first);
	ASSERT_EQ("by//nature", s.second);
}

TEST(BufferBase, beginsBufferRef)
{
	BufferRef b("foo.bar");

	ASSERT_TRUE(b.begins(BufferRef()));
	ASSERT_TRUE(b.begins(BufferRef("foo.")));
	ASSERT_FALSE(b.begins(BufferRef("foo.bar.bak")));
}

TEST(BufferBase, endsBufferRef)
{
	BufferRef b("nature.out");

	ASSERT_TRUE(b.ends(BufferRef()));
	ASSERT_TRUE(b.ends(BufferRef(".out")));
	ASSERT_FALSE(b.ends(BufferRef("long.nature.out")));
}
// }}}
// {{{ BufferRef
TEST(BufferRef, CtorEmpty)
{
	BufferRef ref;
	ASSERT_EQ(0, ref.size());
}

TEST(BufferRef, CtorExplicit)
{
	BufferRef ref("12345", 5);
	ASSERT_EQ(5, ref.size());
	ASSERT_EQ(0, strcmp("12345", ref.data()));
}

TEST(BufferRef, CtorPodType)
{
	BufferRef ref("12345");
	ASSERT_EQ(5, ref.size());
	ASSERT_EQ(0, strcmp("12345", ref.data()));
}

TEST(BufferRef, CtorBufferRef)
{
	BufferRef ref("12345");
	ASSERT_EQ(5, ref.size());
	ASSERT_EQ(0, strcmp("12345", ref.data()));

	BufferRef cpy(ref);
	ASSERT_EQ(5, cpy.size());
	ASSERT_EQ(ref.data(), cpy.data());
}

TEST(BufferRef, AssignBufferRef)
{
	BufferRef ref("12345");
	ASSERT_EQ(5, ref.size());
	ASSERT_EQ(0, strcmp("12345", ref.data()));

	BufferRef cpy;
	cpy = ref;
	ASSERT_EQ(5, cpy.size());
	ASSERT_EQ(ref.data(), cpy.data());
}
// }}} 
// {{{ MutableBuffer
TEST(MutableBuffer, ShlPod)
{
	Buffer b;
	b.push_back("12345");
	ASSERT_EQ(5, b.size());
	ASSERT_EQ("12345", b);
}
// }}}
// {{{ FixedBuffer
TEST(FixedBuffer, test1)
{
	char buf[80] = {"Hello, World"};
	FixedBuffer b(buf, sizeof(buf), 12);
	ASSERT_EQ("Hello, World", b);
	b.clear();
	b.push_back("blah");
	ASSERT_EQ("blah", b);
}
// }}}
// {{{ Buffer
TEST(Buffer, Empty)
{
	Buffer b;
	ASSERT_EQ(true, b.empty());
	ASSERT_EQ(0, b.size());
}

TEST(Buffer, initialCapacity)
{
	Buffer b("foo.bar");
	ASSERT_EQ(7, b.capacity());
}

TEST(Buffer, slice)
{
	Buffer b("foo.bar");
	ASSERT_EQ("foo.bar", b.slice());
	ASSERT_EQ("foo.bar", b.slice(0));
	ASSERT_EQ("foo", b.slice(0, 3));
	ASSERT_EQ("bar", b.slice(4));
	ASSERT_EQ("bar", b.slice(4, 3));
	ASSERT_EQ("b", b.slice(4, 1));
}
// }}}
