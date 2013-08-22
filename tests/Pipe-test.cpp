/* <src/x0d.cpp>
 *
 * This file is part of the x0 web server project and is released under GPL-3.
 * http://www.xzero.io/
 *
 * (c) 2009-2013 Christian Parpart <trapni@gmail.com>
 */

#include <gtest/gtest.h>
#include <xio/KernelBuffer.h>

using namespace xio;

TEST(KernelBufferTest, Empty)
{
	KernelBuffer pp;
	ASSERT_TRUE(pp.empty());
	ASSERT_FALSE(!pp.empty());
	ASSERT_EQ(0, pp.size());
}

TEST(KernelBufferTest, Pass)
{
	std::string msg("Hello");
	KernelBuffer pp;

	// writing to pipe
	ASSERT_EQ(5, pp.write(msg.data(), msg.size()));
	ASSERT_EQ(5, pp.size());

	// reading from pipe
	char buf[80];
	ASSERT_EQ(5, pp.read(buf, sizeof(buf)));
	ASSERT_EQ(0, memcmp(buf, msg.data(), 5));

	// size should be 0 after having read
	ASSERT_EQ(0, pp.size());
}
