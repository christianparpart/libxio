#include <xio/InetServer.h>
#include <xio/Socket.h>
#include <xio/ChunkedStream.h>
#include <xio/BufferStream.h>
#include <xio/Pipe.h>
#include <xio/Buffer.h>
#include <xio/File.h>
#include <ev++.h>
#include <pthread.h>

using namespace xio;

void* client_run(void*)
{
	ev::loop_ref loop = ev::dynamic_loop();
	File file("/etc/passwd");
	auto fs = file.source();

	Buffer buf;
	buf.printf("POST / HTTP/1.1\r\n");
	buf.printf("Content-Length: %zu\r\n", fs->size());
	buf.printf("\r\n");

	Socket client(loop);
	if (!client.open(IPAddress("127.0.0.1"), 8089))
		perror("client: open");

	client.write(buf.data(), buf.size());
	client.write(fs.get(),  fs->size());

	return NULL;
}

int main()
{
	ev::loop_ref loop = ev::default_loop(0);
	Buffer buf;
	buf.reserve(32 * 1024);

	auto server = new InetServer(loop);
	server->open(IPAddress("0.0.0.0"), 8089);
	if (!server->isOpen()) {
		perror("server.open()");
	}

	pthread_t client_thread;
	int rv = pthread_create(&client_thread, NULL, client_run, NULL);
	if (rv < 0) {
		perror("pthread_create");
		exit(1);
	}

	if (Socket* cs = server->acceptOne()) {
		ssize_t n = cs->read(buf, 39);
		write(STDOUT_FILENO, buf.data(), buf.size());
		buf.clear();

		Pipe pipe;
		do {
			n = cs->read(&pipe, 1024);
			pipe.read(STDOUT_FILENO, pipe.size());
		} while (n > 0);

//		cs->read(chunkedBuffer, Stream::MOVE);

		delete cs;
	}

	pthread_join(client_thread, NULL);

	return 0;
}
