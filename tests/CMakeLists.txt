add_definitions(
	-Wall -Wno-deprecated
	-pthread
	-std=c++0x
)

if(BUILD_TESTS)

add_executable(xiotest xiotest.cpp
	Pipe-test.cpp
	Buffer-test.cpp
	BufferStream-test.cpp
)

target_link_libraries(xiotest xio gtest)

add_custom_target(test
	DEPENDS xiotest
	COMMAND ./xiotest ${CMAKE_CURRENT_SOURCE_DIR}
)

endif(BUILD_TESTS)
