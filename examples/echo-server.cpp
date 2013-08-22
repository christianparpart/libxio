// echo-server [-u path] [-b ipaddr] [-p port]

#include <xio/InetServer.h>
#include <xio/IPAddress.h>
#include <xio/Socket.h>

#include <memory>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <ev++.h>

using namespace xio;

class Server
{
public:
	explicit Server(ev::loop_ref loop) : loop_(loop) {}
	~Server() {}

	bool setup();

private:
	void accept(Socket* cli, ServerSocket* srv);

private:
	ev::loop_ref loop_;
	ServerSocket* listener_;
};

bool Server::setup()
{
	auto inet = new InetServer(loop_);
	inet->callback = std::bind(&Server::accept, this, std::placeholders::_1, std::placeholders::_2);

	if (!inet->open(IPAddress("0.0.0.0"), 3000, O_NONBLOCK | O_CLOEXEC)) {
		fprintf(stderr, "Failed to setup server socket. %s\n", strerror(errno));
		return false;
	}

	listener_ = inet;

	return true;
}

void Server::accept(Socket* client, ServerSocket* /*local*/)
{
	printf("New client connected.\n");
	int* i = new int(0);

	client->on(Socket::READ, TimeSpan::fromSeconds(10), [client, i](int re) {
		if (re & Socket::TIMEOUT) {
			printf("Remote endpoint timed out.\n");
			delete client;
		} else {
			char buf[1024];
			ssize_t n = client->read(buf, sizeof(buf));
			printf("Client read %d-th chunk. %s", ++*i, buf); // buf is to contain \r\n (from telnet)
			if (n > 0) {
				client->write(buf, n);  // we ignore any kind of write errors here
				client->restart();      // restart I/O timer
			} else if (n == 0) {
				printf("Remote endpoint closed.\n");
				delete client;
				delete i;
			} else {
				printf("Remote endpoint read error. %s\n", strerror(errno));
				delete client;
				delete i;
			}
		}
	});
}

int main(int argc, const char* argv[])
{
	ev::loop_ref loop(ev::default_loop(0));
	Server srv(loop);

	if (!srv.setup())
		return 1;

	loop.run();

	return 0;
}
