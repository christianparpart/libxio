#include <gtest/gtest.h>
#include <xio/Pipe.h>
#include <xio/ChunkedStream.h>
#include <xio/BufferStream.h>

using namespace xio;

TEST(ChunkedStream, empty)
{
	BufferStream stream;
	ASSERT_TRUE(stream.empty());
	ASSERT_EQ(0, stream.size());
	ASSERT_EQ(0, stream.capacity());

	char buf[10] = "123456789";
	size_t n = stream.read(buf, sizeof(buf));
	ASSERT_EQ(0, n);
	ASSERT_EQ("123456789", std::string(buf)); // did not get modified
}

TEST(ChunkedStream, write1)
{
	BufferStream stream;
	ssize_t n = stream.write("foo", 4);
	ASSERT_EQ(4, n);
	ASSERT_EQ(4, stream.size());

	char out[10];
	n = stream.read(out, sizeof(out));

	ASSERT_EQ(4, n);
	ASSERT_EQ(0, strcmp(out, "foo"));
}

TEST(ChunkedStream, write2)
{
	BufferStream stream;
	stream.write("foo", 3);
	ssize_t n = stream.write("bar", 4);
	ASSERT_EQ(4, n);
	ASSERT_EQ(7, stream.size());

	char out[10];
	n = stream.read(out, sizeof(out));

	ASSERT_EQ(7, n);
	ASSERT_EQ(0, strcmp(out, "foobar"));
}


TEST(ChunkedStream, read2)
{
	BufferStream stream;
	stream.write("foobar", 6);

	char out[10];
	ssize_t n = stream.read(out, 3);
	ASSERT_EQ(3, n);
	out[3] = '\0';
	ASSERT_EQ(0, strcmp(out, "foo"));

	n = stream.read(out, 3);
	ASSERT_EQ(3, n);
	out[3] = '\0';
	ASSERT_EQ(out, std::string("bar"));
}

TEST(ChunkedStream, pipe_move1)
{
	ssize_t n;
	char buf[10];
	Pipe pipe;
	ChunkedStream stream;

	n = pipe.write("foo", 3);
	ASSERT_EQ(3, n);

	n = stream.write(&pipe, pipe.size(), Stream::MOVE);
	ASSERT_EQ(3, n);

	n = stream.read(buf, sizeof(buf));
	ASSERT_EQ(3, n);
	buf[n] = '\0';
	ASSERT_EQ(buf, std::string("foo"));
}

TEST(ChunkedStream, pipe_copy1)
{
	ssize_t n;
	char buf[10];
	Pipe pipe;
	ChunkedStream stream;

	n = pipe.write("foo", 3);
	ASSERT_EQ(3, n);

	// write() ->
	// source.move(target);
	// source.copy(target);
	//
	// special streams:
	// - socket
	// - pipe

	n = stream.write(&pipe, pipe.size(), Stream::COPY);
	ASSERT_EQ(3, n);

	n = stream.read(buf, sizeof(buf));
	ASSERT_EQ(3, n);
	buf[n] = '\0';
	ASSERT_EQ(buf, std::string("foo"));
}
