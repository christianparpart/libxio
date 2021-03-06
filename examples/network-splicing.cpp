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

const char* bindaddr = "0.0.0.0";
const char* ipaddr = "127.0.0.1";
int port = 8089;

void* client_run(void*)
{
	ev::loop_ref loop = ev::dynamic_loop();
	File file("/etc/issue");
	auto fs = file.open(O_RDONLY);

	Buffer buf;
	buf.printf("POST / HTTP/1.1\r\n");
	buf.printf("Content-Length: %zu\r\n", fs->size());
	buf.printf("\r\n");

	Socket client(loop);
	if (client.open(IPAddress(ipaddr), port)) {
		client.write(buf.data(), buf.size());
		client.write(fs.get(),  fs->size());
	} else {
		perror("client: open");
	}

	return NULL;
}

/**
 * Receive data from one client, connect a backend, and transfers client's data
 * directly to the backend server <b>without</b> userspace copying.
 */
void* proxy_run(void* p)
{
	Socket* client = (Socket*) p;
	ev::loop_ref loop = ev::dynamic_loop();

	Socket backend(loop);
	if (!backend.open(IPAddress(ipaddr), port))
		perror("proxy: open");

	Pipe pipe;
	for (;;) {
		if (client->read(&pipe, 1024) > 0) {
			backend.write(&pipe, pipe.size());
		} else {
			break;
		}
	}

	delete client;
	return NULL;
}

int main(int argc, const char* argv[])
{
	if (argc == 4) {
		port = atoi(argv[1]);
		ipaddr = argv[2];
		bindaddr = argv[3];
	}

	ev::loop_ref loop = ev::default_loop(0);

	InetServer server(loop);
	server.open(IPAddress(bindaddr), port);
	if (!server.isOpen()) {
		perror("server.open()");
		return 1;
	}

	pthread_t client_thread;
	int rv = pthread_create(&client_thread, NULL, client_run, NULL);
	if (rv < 0) {
		perror("pthread_create");
		return 1;
	}

	pthread_t proxy_thread;
	for (int i = 0; i < 2; ++i) {
		Socket* cs = server.acceptOne();
		if (!cs) {
			perror("accept");
			return 1;
		}

		if (i == 0) {
			// first client gets proxied
			pthread_create(&proxy_thread, NULL, proxy_run, cs);
		} else {
			// second client gets dumped to terminal (partly reading into buf, remaining data via pipe)
			Buffer buf;
			cs->read(buf, 39);
			write(STDOUT_FILENO, buf.data(), buf.size());

			Pipe pipe;
			for (;;) {
				if (cs->read(&pipe, 1024) > 0) {
					pipe.read(STDOUT_FILENO, pipe.size());
				} else {
					break;
				}
			}
			// TODO: cs->read(chunkedBuffer, Stream::MOVE);
			delete cs;
		}
	}

	pthread_join(client_thread, NULL);
	pthread_join(proxy_thread, NULL);

	return 0;
}
