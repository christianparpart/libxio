// echo-server [-u path] [-b ipaddr] [-p port]

#include <xio/InetServer.h>
#include <xio/IPAddress.h>
#include <xio/BufferStream.h>
#include <xio/Socket.h>

#include <memory>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <ev++.h>

using namespace xio;

class Server {
public:
	explicit Server(ev::loop_ref loop) :
		loop_(loop),
		listener_(),
		sigint_(loop_)
	{}

	~Server();

	bool setup();
	void stop();

private:
	void accept(Socket* cli, ServerSocket* srv);
	void sig(ev::sig&, int);

	ev::loop_ref loop_;
	std::shared_ptr<ServerSocket> listener_;
	ev::sig sigint_;
};

class Client {
public:
	Client(Server* server, Socket* socket);
	~Client();

	void close();

private:
	Server* server_;
	Socket* socket_;
	BufferStream writeBuffer_;
	bool close_;
};

Server::~Server()
{
	loop_.ref();
	sigint_.stop();
}

bool Server::setup()
{
	auto inet = std::make_shared<InetServer>(loop_);
	inet->callback = std::bind(&Server::accept, this, std::placeholders::_1, std::placeholders::_2);

	if (!inet->open(IPAddress("0.0.0.0"), 3000, O_NONBLOCK | O_CLOEXEC)) {
		fprintf(stderr, "Failed to setup server socket. %s\n", strerror(errno));
		return false;
	}
	printf("Listening on %s:%d\n", "0.0.0.0", 3000);

	listener_ = inet;

	sigint_.set<Server, &Server::sig>(this);
	sigint_.start(SIGINT);
	loop_.unref();

	return true;
}

void Server::sig(ev::sig& sig, int revents)
{
	printf("Stop signalled.\n");
	stop();
}

void Server::stop()
{
	listener_->stop();
}

void Server::accept(Socket* socket, ServerSocket* /*local*/)
{
	new Client(this, socket);
}

Client::Client(Server* server, Socket* socket) :
	server_(server),
	socket_(socket),
	writeBuffer_(),
	close_(false)
{
	printf("New client connected.\n");

	auto i = std::make_shared<int>(0);

	socket->on(Socket::READ, TimeSpan::fromSeconds(10), [i, this](int re) {
		if (re & Socket::TIMEOUT) {
			printf("Remote endpoint timed out.\n");
			delete this;
			return;
		}

		if (re & Socket::READ) {
			char buf[1024];
			ssize_t n = socket_->read(buf, sizeof(buf));
			if (n > 0) {
				// buf is to contain \r\n (from telnet)
				buf[n] = 0;
				printf("Client read %d-th chunk. %s", ++*i, buf);
				writeBuffer_.write(buf, n);
				socket_->watch(Socket::READ | Socket::WRITE);

				if (strncmp(buf, "..", n - 2) == 0) {
					close();
					server_->stop();
				} else if (strncmp(buf, ".", n - 2) == 0) {
					close_ = true;
				}
			} else if (n == 0) {
				printf("Remote endpoint closed.\n");
				delete this;
				return;
			} else {
				printf("Remote endpoint read error. %s\n", strerror(errno));
				delete this;
				return;
			}
		}

		if (re & Socket::WRITE) {
			ssize_t n = socket_->write(writeBuffer_.data() + writeBuffer_.readOffset(), writeBuffer_.size());
			if (n > 0) {
				writeBuffer_.shift(n);
			} else if (n < 0) {
				perror("write");
				delete this;
				return;
			}

			if (writeBuffer_.empty()) {
				if (close_) {
					delete this;
					return;
				} else {
					socket_->watch(Socket::READ);
				}
			}
		}
	});
}

void Client::close()
{
	close_ = true;
}

Client::~Client()
{
	printf("Client disconnected.\n");
	delete socket_;
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
