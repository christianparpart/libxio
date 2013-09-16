#include <xio/Stream.h>
#include <xio/File.h>
#include <xio/FileStream.h>
//#include <xio/Filter.h>
#include <memory>

using namespace xio;

int main(int argc, char* argv[])
{
	File fin("/dev/stdin");
	File fout("/dev/stdout");
	auto in = fin.source();
	auto out = fout.sink();
	char buf[1024];
	ssize_t n;

	while ((n = in->read(buf, sizeof(buf))) > 0)
		out->write(buf, n);

	return 0;
}
