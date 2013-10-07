# TODO

- I/O filter example (upper/lower case, compress/uncompress)
- chunked stream example

# Stackoverflow question

## How to design a C++ Buffer API using templates.

I would like to write a C++ API for managing char buffers of various types
*without* duplicating too much of the code.

So my requirement is to support the following buffer types:

### fixed-size immutable unmanaged buffer {`char* data; size_t size;`}

A basic buffer type pointing natively to some native memory plus the size.
This type implements functions like `begins()`, `ends()`, and everything else
you can do on a immutable buffer.

### fixed-size mutable unmanaged buffer {`char* data; size_t size; size_t capacity;`}

Same as the first type but mutable and adds a capacity integer to represent the total
buffer size while `size` itself represents the actual used buffer size (to implement
methods like `size()`, `clear()` and support incremental population via `push_back()` etc.)

So we could inherit from the first type.

### dynamic-size mutable managed buffer {`char* data; size_t size; size_t capacity;`}

Same as the second type, but including memory management, that is, `malloc()`, `realloc()` and `free()`.

So we could inherit from the second type.

### sub-view buffer into managed buffer {`Buffer* data; size_t offset; size_t size;`}

Basically, the 4th type of buffer is exactly like the first, except, that it
refers to another buffer instead of some raw memory region. This type is important
as it points to some 3rd-type buffer and thus, the data base pointer is subject to
change, as `realloc()` is not guarranteed to keep the memory address, which would
invalidate all buffer views of this (4ths) type, that is why we keep a
pointer to the buffer itself plus an offset into it and size.

So we cannot inherit from the first but instead specialize it.
