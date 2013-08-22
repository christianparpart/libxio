#include <xio/Stream.h>
#include <xio/ChunkedBuffer.h>
#include <xio/MemoryBuffer.h>
#include <xio/KernelBuffer.h>
#include <xio/Socket.h>
#include <xio/IPAddress.h>

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ev++.h>

using namespace xio;

int main(int argc, const char* argv[])
{
	ev::loop_ref loop(ev::default_loop(0));

	Socket socket(loop);
	Socket::State ss = socket.open(IPAddress("127.0.0.1"), 8080);
	const char* sstr[] = { "Closed", "Connecting", "Handshake", "Operational" };
	printf("** socket state: %s\n", sstr[(int)ss]);

	socket.on(Socket::CONNECTED, TimeSpan::fromSeconds(10), [&](int re) {
		if (re & Socket::TIMEOUT) {
			printf("** Connecting timed out\n");
			socket.stop();
		} else {
			socket.on(Socket::WRITE, TimeSpan::fromSeconds(10), [&](int re) {
				if (re & Socket::TIMEOUT) {
					printf("** waiting for write-readiness timed out\n");
				} else {
					printf("** writing request\n");

					socket.write(
						"GET / HTTP/1.1\r\n"
						"Host: localhost\r\n"
						"Connection: close\r\n"
						"\r\n");

					socket.on(Socket::READ, TimeSpan::fromSeconds(10), [&](int re) {
						if (re & Socket::TIMEOUT) {
							socket.stop();
							printf("** waiting for response timed out\n");
						} else {
							printf("** receiving response.\n");
							char buf[1024 * 64];
							ssize_t n = socket.read(buf, sizeof(buf));
							if (n > 0) {
								buf[n] = 0;
								printf("** Read %zi bytes\n", n);
								printf("%s\n", buf);
							} else if (n == 0) {
								printf("** Remote endpoint closed.\n");
								socket.stop();
							} else {
								printf("** Error reading. %s\n", strerror(errno));
								socket.stop();
							}
						}
					});
				}
			});
		}
	});

	loop.run();

	return 0;
}
