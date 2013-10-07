#include <xio/Stream.h>
#include <xio/File.h>
#include <xio/FileStream.h>
#include <xio/FilterStream.h>
#include <memory>

using namespace xio;

int main(int argc, char* argv[])
{
	File fin("/dev/stdin");
	File fout("/dev/stdout");
	auto in = fin.open(O_RDONLY);
	auto out = fout.open(O_WRONLY);
	char buf[1024];
	ssize_t n;

//	in = std::unique_ptr<FilterStream>(new FilterStream(std::move(in)));

	while ((n = in->read(buf, sizeof(buf))) > 0)
		out->write(buf, n);

	return 0;
}
