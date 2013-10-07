/* <src/x0d.cpp>
 *
 * This file is part of the x0 web server project and is released under GPL-3.
 * http://www.xzero.io/
 *
 * (c) 2009-2013 Christian Parpart <trapni@gmail.com>
 */

#include <gtest/gtest.h>
#include <xio/Pipe.h>
#include <fcntl.h>

using namespace xio;

TEST(Pipe, Empty)
{
	Pipe pp;
	ASSERT_TRUE(pp.empty());
	ASSERT_FALSE(!pp.empty());
	ASSERT_EQ(0, pp.size());
}

TEST(Pipe, Pass)
{
	std::string msg("Hello");
	Pipe pp;

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

TEST(Pipe, clear)
{
	char buf[4096] = {0};
	Pipe p;
	ssize_t n;

	n = p.write(buf, sizeof(buf));
	ASSERT_EQ(sizeof(buf), n);
	ASSERT_EQ(sizeof(buf), p.size());

	n = p.write(buf, sizeof(buf));
	ASSERT_EQ(sizeof(buf), n);
	ASSERT_EQ(sizeof(buf) * 2, p.size());

	p.clear();

	ASSERT_EQ(0, p.size());
}

TEST(Pipe, WritePipeMOVE)
{
	Pipe a(O_NONBLOCK);
	Pipe b(O_NONBLOCK);
	ssize_t n;
	char buf[10];

	a.write("foo", 3);
	n = b.write(&a, a.size(), Stream::MOVE);
	ASSERT_EQ(3, n);
	ASSERT_EQ(0, a.size());
	ASSERT_EQ(3, b.size());

	n = a.read(buf, 3);
	ASSERT_EQ(-1, n);
	ASSERT_EQ(EAGAIN, errno);

	n = b.read(buf, 3);
	ASSERT_EQ(3, n);
	buf[n] = '\0';
	ASSERT_EQ(std::string("foo"), buf);
}

TEST(Pipe, WritePipeCOPY)
{
	Pipe a(O_NONBLOCK);
	Pipe b(O_NONBLOCK);
	ssize_t n;
	char buf[10];

	a.write("foo", 3);
	n = b.write(&a, a.size(), Stream::COPY);
	ASSERT_EQ(3, n);
	ASSERT_EQ(3, a.size());
	ASSERT_EQ(3, b.size());

	n = a.read(buf, 3);
	ASSERT_EQ(3, n);
	buf[n] = '\0';
	ASSERT_EQ(std::string("foo"), buf);

	n = b.read(buf, 3);
	ASSERT_EQ(3, n);
	buf[n] = '\0';
	ASSERT_EQ(std::string("foo"), buf);
}

