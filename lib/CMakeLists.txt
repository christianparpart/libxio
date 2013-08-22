add_definitions(
	-Dlinux=1
	-Wall -Wno-deprecated
	-pthread
	-std=c++0x
	-DPACKAGE_VERSION="${PACKAGE_VERSION}"
)

add_definitions(-fvisibility=hidden -fvisibility-inlines-hidden -DBUILD_XIO=1)

add_library(xio SHARED
	IPAddress.cpp
	TimeSpan.cpp
	DateTime.cpp
	Stream.cpp
	MemoryBuffer.cpp
	KernelBuffer.cpp
	SocketDriver.cpp
	Socket.cpp
	ServerSocket.cpp
	ChunkedBuffer.cpp
)

target_link_libraries(xio
	pthread
	${EV_LIBRARIES}
	${SD_LIBRARIES}
)

set_target_properties(xio PROPERTIES VERSION ${PACKAGE_VERSION})

install(TARGETS xio DESTINATION ${LIBDIR})
