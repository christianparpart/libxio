add_definitions(-Wall -Wno-deprecated -DPACKAGE_VERSION="${PACKAGE_VERSION}")
add_definitions(-fvisibility=hidden -fvisibility-inlines-hidden -DBUILD_XIO=1)
set(CMAKE_CXX_FLAGS "-std=c++11 -pthread")

add_library(xio SHARED
	Buffer.cpp Stream.cpp Pipe.cpp BufferStream.cpp ChunkedStream.cpp TimeSpan.cpp
	DateTime.cpp IPAddress.cpp FileStream.cpp File.cpp SocketDriver.cpp Socket.cpp
	ServerSocket.cpp InetServer.cpp UnixServer.cpp FilterStream.cpp Filter.cpp)

target_link_libraries(xio pthread ${EV_LIBRARIES} ${SD_LIBRARIES})
set_target_properties(xio PROPERTIES VERSION ${PACKAGE_VERSION})
install(TARGETS xio DESTINATION ${LIBDIR})
