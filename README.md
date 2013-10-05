# libxio

## Motivation

The idea behind libxio is, to basically privde a better I/O subsystem as I have
designed so far for x0. However, this included also copying (and partly redesigning)
some other parts of the base APIs, too; reason enough to make it a fully featured
base framework an application can depend on.

## Class Diagram

- `BufferRef` - unmanaged immutable buffer
- `Buffer` - managed mutable buffer
- `BufferSlice` - safe slice into a managed mutable buffer
- `FixedBuffer` - unmanaged mutable buffer
- `DateTime` - date/time
- `TimeSpan` - a time span / duration
- `File` - regular file object
- `FileMgr` - cache/lookup manager for file objects
- `SocketDriver`
- `ServerSocket`
  - `InetServer` - TCP/IP server
  - `UnixServer` - `AF_UNIX` server
- `Stream`
  - `Socket` - (TCP) streaming socket
  - `Pipe` - kernel pipe
  - `BufferStream` - userspace buffer stream
  - `ChunkedStream` - composable stream, with userspace-/ kernelspace buffer chunks
  - `FilterStream` - fitlerable stream
- `Filter` - abstract filter
  - `NullFilter`
  - ...

## Byte Buffers API

## Chunked Buffers API

The goal of *Chunked Buffers API* is to allow direct memory access on chunks as well
as support to anonymous payloads that just needs to be transferred from one end
to another.

Consider an HTTP message where you must analyze the request line and request headers byte-wise
but you can just ignore the actual contents of the request body and pass it as-is to the 
destination of interest, i.e. a backend server.

In this case you want to avoid unnecessary copying of the request body from kernel space 
to userspace and back into kernel space. It would be ideal to just leave the buffer in
kernel space by abusing the characteristics of system pipes in combination of
some kernel system calls.

- `ChunkReader` - abstracts a chunk for read operations
- `Chunk`: `ChunkReader` - a chunk of bytes, adds write operations
  - `MemoryChunk` - a raw memory chunk
  - `Pipe` - implements a chunk as system pipe
  - `ChunkedBuffer`

## Socket API

- `SocketDriver` - socket factory to create new `Socket` instances
- `ServerSocket` - server listener socket
- `Socket` - some socket endpoint

### Evented Socket Processing

    Socket socket;

    void watch(Event::Mode mode) {
      socket.on(mode, TimeSpan::fromSeconds(60), [&]() { io(); });
    }

    void io() {
        if (socket.isError()) {
            close();
            return;
        }

        if (socket.isReadable())
            readSome();

        if (socket.isWriteable())
            writeSome();
    });

## Stream API

- `StreamReader`: exposes only read operations on a stream
- `Stream`
  - `FileStream` - local file
  - `Socket` - network socket
  - `Pipe` - system pipe
  - `BufferStream` - memory buffer
  - `OpaqueBuffer`
- `StreamVisitor`
- `File`
- `FileMgr`

## The Splice Efford

The idea behind is to support splicing from a socket into this object and then back into the next socket.
So when filling this stream we must know in advance whether or not we actually need the data to inspect
in form of memory or not.

### Use Cases

HTTP client request bodies do usually *ONLY* exist when it is being proxied to some backend service,
and thus makes most sense to make use of `splice()` here.
The only usecase for me is a native WebDAV plugin that directly writes the request body into a local file,
something, that cannot be optimized by splice() or similar (AFAIK).

1. Server reads from client socket some buffer in COPY mode, analyzes protocol header and payload length.
2. Server reads (remaining) payload from client in MOVE mode.
3. Server composes backend request by writing request to backend socket and then transfers the client's payload to it.

### Pseudo Code

- read chunk of size N into memory
- read chunk of size N into pipe

    class Chunk {
    public:
        virtual ssize_t read(int fd, size_t size) = 0;
        virtual ssize_t read(char* buf, size_t size) = 0;
    protected:
        virtual ssize_t write(int fd, size_t size) = 0;
        virtual ssize_t write(char* buf, size_t size) = 0;
    };

    class ChunkedStream {
        class Memory;
        class Pipe;

        ssize_t push_back(int fd, size_t size, bool opaque);
        ssize_t pop_front(int fd, size_t size);
        Chunk* front();
    };

## STL integration brainstorming

    class iochan {
        virtual ssize_t write(const char* buf, size_t len);
        virtual ssize_t read(const char* buf, size_t len);
    }
    class file_channel : public iochan;
    class pipe_channel : public iochan;
    class buffer_channel : public iochan;
    class socket_channel : public iochan;

    class listener {
        void bind(const ipaddr& ip, int port);
        void listen(int backlog);
    }
    class socket_client {
        void connect(const ipaddr& ip, int port);
    };
    class tcp_server : public listener_socket;
    class udp_server : public listener_socket;


## See Also

- Java NIO [http://en.wikipedia.org/wiki/New_I/O]

