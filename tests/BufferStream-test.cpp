#include <gtest/gtest.h>
#include <xio/Pipe.h>
#include <xio/ChunkedStream.h>
#include <xio/BufferStream.h>

using namespace xio;

TEST(BufferStream, empty)
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

TEST(BufferStream, write1)
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
