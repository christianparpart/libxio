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
	printf("client: started\n");
	ev::loop_ref loop = ev::dynamic_loop();
	File file("/etc/issue");
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

	client.read(buf);

	printf("client done\n");
	return NULL;
}

int main()
{
	ev::loop_ref loop = ev::default_loop(0);

	auto server = new InetServer(loop);
	server->open(IPAddress("0.0.0.0"), 8089);
	if (!server->isOpen()) {
		perror("server.open()");
	}

	pthread_t client;
	int rv = pthread_create(&client, NULL, client_run, NULL);
	if (rv < 0) {
		perror("pthread_create");
		exit(1);
	}

	if (Socket* cs = server->acceptOne()) {
		printf("server: received client connect\n");
		cs->write("Hello\r\n", 7);
		delete cs;
	}

	pthread_join(client, NULL);

	return 0;
}
