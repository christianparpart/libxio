cmake_minimum_required(VERSION 2.8)

project(xio)
set(PACKAGE_VERSION "0.1.0")

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(FindPkgConfig)
include(FindDoxygen)
include(CheckIncludeFiles)
include(CheckFunctionExists)
include(CheckVariableExists)
include(CheckTypeSize)
include(CheckLibraryExists)
include(CheckCSourceCompiles)
include(CMakeDetermineCCompiler)

option(WITH_INOTIFY "Build with inotify support [default: on]" ON)
option(WITH_SSL "Builds with SSL support [default: on]" OFF)

add_definitions(-Wall -Wno-variadic-macros)

# we need the following definitions in order to get some special
# OS-level features like posix_fadvise() or readahead().
add_definitions(-DXOPEN_SOURCE=600)
add_definitions(-DGNU_SOURCE)

# enforce 64bit i/o operations, even on 32bit platforms
add_definitions(-D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGE_FILES)

CHECK_INCLUDE_FILES(fcntl.h HAVE_FCNTL_H)
CHECK_INCLUDE_FILES(netdb.h HAVE_NETDB_H)
CHECK_INCLUDE_FILES(gtest/gtest.h HAVE_GTEST_GTEST_H)

CHECK_INCLUDE_FILES(sys/sendfile.h HAVE_SYS_SENDFILE_H)
CHECK_FUNCTION_EXISTS(sendfile HAVE_SENDFILE)
CHECK_FUNCTION_EXISTS(posix_fadvise HAVE_POSIX_FADVISE)
CHECK_FUNCTION_EXISTS(readahead HAVE_READAHEAD)

CHECK_INCLUDE_FILES(sys/resource.h HAVE_SYS_RESOURCE_H)
CHECK_INCLUDE_FILES(sys/mman.h HAVE_SYS_MMAN_H)
CHECK_INCLUDE_FILES(sys/limits.h HAVE_SYS_LIMITS_H)
CHECK_INCLUDE_FILES(pwd.h HAVE_PWD_H)
CHECK_INCLUDE_FILES(syslog.h HAVE_SYSLOG_H)
CHECK_INCLUDE_FILES(uuid/uuid.h HAVE_UUID_UUID_H)
CHECK_INCLUDE_FILES(sys/utsname.h HAVE_SYS_UTSNAME_H)
CHECK_INCLUDE_FILES(ldap.h HAVE_LDAP_H)

CHECK_FUNCTION_EXISTS(fork HAVE_FORK)
CHECK_FUNCTION_EXISTS(chroot HAVE_CHROOT)
CHECK_FUNCTION_EXISTS(pathconf HAVE_PATHCONF)
CHECK_FUNCTION_EXISTS(accept4 HAVE_ACCEPT4)

if(WITH_INOTIFY)
	CHECK_INCLUDE_FILES(sys/inotify.h HAVE_SYS_INOTIFY_H)
	if(HAVE_SYS_INOTIFY_H)
		CHECK_FUNCTION_EXISTS(inotify_init1 HAVE_INOTIFY_INIT1)
	endif(HAVE_SYS_INOTIFY_H)
endif(WITH_INOTIFY)

# TODO dynamic check for tbb
set(TBB_LIBRARIES tbb)

# ---------------------------------------------------------------------------
include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# ev
find_package(EV)
include_directories(${EV_INCLUDE_DIR})
link_directories(${EV_LIBRARY_DIR})
add_definitions(${EV_CPPFLAGS})

# {{{ path prefixes
# ---------------------------------------------------------------------------
if (DEFINED CMAKE_INSTALL_PREFIX)
	set(SYSCONFDIR ${CMAKE_INSTALL_PREFIX}/etc CACHE PATH "x0d configuration directory")
	set(INCLUDEDIR ${CMAKE_INSTALL_PREFIX}/include CACHE PATH "x0 include directory")
	set(LIBDIR ${CMAKE_INSTALL_PREFIX}/lib CACHE PATH "x0 library directory")
	set(LOGDIR ${CMAKE_INSTALL_PREFIX}/var/log CACHE PATH "log file directory")
endif()

if (DEFINED SYSCONFDIR)
	set(SYSCONFDIR ${SYSCONFDIR} CACHE PATH "configuration directory")
else()
	set(SYSCONFDIR ${CMAKE_INSTALL_PREFIX}/etc CACHE PATH "configuration directory")
endif()

if (DEFINED INCLUDEDIR)
	set(INCLUDEDIR ${INCLUDEDIR} CACHE PATH "path to library includes")
else()
	set(INCLUDEDIR ${CMAKE_INSTALL_PREFIX}/include CACHE PATH "path to library includes")
endif()

if (DEFINED LIBDIR)
	set(LIBDIR ${LIBDIR} CACHE PATH "path to program libraries")
else()
	set(LIBDIR ${CMAKE_INSTALL_PREFIX}/lib CACHE PATH "path to program libraries")
endif()

if (DEFINED LOGDIR)
	set(LOGDIR ${LOGDIR} CACHE PATH "log file directory")
else()
	set(LOGDIR ${CMAKE_INSTALL_PREFIX}/var/log CACHE PATH "log file directory")
endif()

set(PLUGINDIR "${LIBDIR}/x0" CACHE PATH "path to x0 plugins")
# ---------------------------------------------------------------------------
# }}}

if(WITH_SSL)
	pkg_check_modules(GNUTLS REQUIRED gnutls>=2.0.0)
endif(WITH_SSL)

option(BUILD_TESTS "Build unit tests [default: off]" OFF)
if(BUILD_TESTS)
	CHECK_INCLUDE_FILES(gtest/gtest.h HAVE_GTEST_GTEST_H)
endif(BUILD_TESTS)

option(BUILD_EXAMPLES "Build examples [default: on]" ON)
if(BUILD_EXAMPLES)
	# no additional requirements yet
endif(BUILD_EXAMPLES)

# systemd support (implemented in support/sd-daemon/)
set(SD_LIBRARIES sd-daemon)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/support/sd-daemon)

configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/include/xio/sysconfig.h.cmake
	${CMAKE_CURRENT_BINARY_DIR}/include/xio/sysconfig.h)

add_definitions(-DHAVE_SYSCONFIG_H)

add_subdirectory(support)
add_subdirectory(lib)
add_subdirectory(tests)
add_subdirectory(examples)

