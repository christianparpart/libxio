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

TEST(BufferTest, Empty)
{
	Buffer b;
	ASSERT_EQ(true, b.empty());
	ASSERT_EQ(0, b.size());
}

TEST(BufferTest, split1)
{
	Buffer b("fnorded,by,nature");
	auto s = b.split(',');
	ASSERT_EQ("fnorded", s.first);
	ASSERT_EQ("by,nature", s.second);
}

TEST(BufferTest, split2)
{
	BufferRef b("fnorded//by//nature");
	auto s = b.split("//");
	ASSERT_EQ("fnorded", s.first);
	ASSERT_EQ("by//nature", s.second);
}
