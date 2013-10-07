add_definitions(
	-Wall -Wno-deprecated
	-pthread
	-std=c++0x
)

if(BUILD_EXAMPLES)

add_executable(xiocp xiocp.cpp)
target_link_libraries(xiocp xio)

add_executable(tcp-client tcp-client.cpp)
target_link_libraries(tcp-client xio)

add_executable(echo-server echo-server.cpp)
target_link_libraries(echo-server xio)

add_executable(network-splicing network-splicing.cpp)
target_link_libraries(network-splicing xio)

add_executable(filter filter.cpp)
target_link_libraries(filter xio)

endif(BUILD_EXAMPLES)
