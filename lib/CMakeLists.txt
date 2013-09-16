add_definitions(
	-Dlinux=1
	-Wall -Wno-deprecated
	-pthread
	-std=c++0x
	-DPACKAGE_VERSION="${PACKAGE_VERSION}"
)

add_definitions(-fvisibility=hidden -fvisibility-inlines-hidden -DBUILD_XIO=1)

add_library(xio SHARED
	Buffer.cpp
	Stream.cpp
	Pipe.cpp
	BufferStream.cpp
	ChunkedStream.cpp
	TimeSpan.cpp
	DateTime.cpp
	IPAddress.cpp
	FileStream.cpp
	File.cpp
	SocketDriver.cpp
	Socket.cpp
	ServerSocket.cpp
	InetServer.cpp
	UnixServer.cpp
	Filter.cpp
	FilterStream.cpp
)

target_link_libraries(xio
	pthread
	${EV_LIBRARIES}
	${SD_LIBRARIES}
)

set_target_properties(xio PROPERTIES VERSION ${PACKAGE_VERSION})

install(TARGETS xio DESTINATION ${LIBDIR})
