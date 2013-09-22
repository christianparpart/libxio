#include <xio/Stream.h>
#include <xio/ChunkedStream.h>
#include <xio/BufferStream.h>
#include <xio/Pipe.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ev++.h>

using namespace xio;

#if 0
void cbtest()
{
	struct stat st;
	int fd = ::open("/etc/issue", O_RDONLY);
	fstat(fd, &st);

	ChunkedBuffer buf;
	buf.write("Hello ");
	buf.write("World");
	buf.write("\n");
	buf.write(fd, st.st_size);
	// writev(iov...)

#if 0
//	for (auto chunk: buf)
//		printf("chunk: <'%p', %zu>\n", chunk.first, chunk.second);

	buf.each([&](const char* data, size_t n) -> bool {
		printf("each: %zu bytes\n", n);
		//::write(1, data, n);
		return true;
	});
#else
	size_t ts = buf.size();
	size_t n = 0;
	while (buf.read(STDOUT_FILENO, 10) > 0)
		++n;
/*
	int ch;
	while ((ch = buf.read()) != -1) {
		fprintf(stdout, "%c", ch);
		++n;
	}
	*/
	fflush(stdout);

	printf("\nprocessed %zu bytes in %zu iterations\n", ts, n);
#endif
}
#endif

int main(int argc, const char* argv[])
{

	return 0;
}
