#pragma once
/* <x0/Defines.h>
 *
 * This file is part of the x0 web server project and is released under LGPL-3.
 * http://www.xzero.io/
 *
 * (c) 2009-2013 Christian Parpart <trapni@gmail.com>
 */

#include <cstddef>
#include <cstring>
#include <cstdio>

#if !defined(XZERO_NDEBUG)
#	include <ev.h> // required for DEBUG()
#endif

// platforms
#if defined(_WIN32) || defined(__WIN32__)
#	define XIO_OS_WIN32 1
//#	define _WIN32_WINNT 0x0510
#else
#	define XIO_OS_UNIX 1
#	if defined(__CYGWIN__)
#		define XIO_OS_WIN32 1
#	elif defined(__APPLE__)
#		define XIO_OS_DARWIN 1 /* MacOS/X 10 */
#	endif
#endif

// api decl tools
#if defined(__GNUC__)
#	define XIO_NO_EXPORT __attribute__((visibility("hidden")))
#	define XIO_EXPORT __attribute__((visibility("default")))
#	define XIO_IMPORT /*!*/
#	define XIO_WARN_UNUSED_RESULT __attribute__((warn_unused_result))
#	define XIO_NO_RETURN __attribute__((no_return))
#	define XIO_DEPRECATED __attribute__((__deprecated__))
#	define XIO_PURE __attribute__((pure))
#	define XIO_PACKED __attribute__((packed))
#	if !defined(likely)
#		define likely(x) __builtin_expect((x), 1)
#	endif
#	if !defined(unlikely)
#		define unlikely(x) __builtin_expect((x), 0)
#	endif
#elif defined(__MINGW32__)
#	define XIO_NO_EXPORT /*!*/
#	define XIO_EXPORT __declspec(export)
#	define XIO_IMPORT __declspec(import)
#	define XIO_WARN_UNUSED_RESULT __attribute__((warn_unused_result))
#	define XIO_NO_RETURN __attribute__((no_return))
#	define XIO_DEPRECATED __attribute__((__deprecated__))
#	define XIO_PURE __attribute__((pure))
#	define XIO_PACKED __attribute__((packed))
#	if !defined(likely)
#		define likely(x) (x)
#	endif
#	if !defined(unlikely)
#		define unlikely(x) (x)
#	endif
#elif defined(__MSVC__)
#	define XIO_NO_EXPORT /*!*/
#	define XIO_EXPORT __declspec(export)
#	define XIO_IMPORT __declspec(import)
#	define XIO_WARN_UNUSED_RESULT /*!*/
#	define XIO_NO_RETURN /*!*/
#	define XIO_DEPRECATED /*!*/
#	define XIO_PURE /*!*/
#	define XIO_PACKED __packed /* ? */
#	if !defined(likely)
#		define likely(x) (x)
#	endif
#	if !defined(unlikely)
#		define unlikely(x) (x)
#	endif
#else
#	warning Unknown platform
#	define XIO_NO_EXPORT /*!*/
#	define XIO_EXPORT /*!*/
#	define XIO_IMPORT /*!*/
#	define XIO_WARN_UNUSED_RESULT /*!*/
#	define XIO_NO_RETURN /*!*/
#	define XIO_DEPRECATED /*!*/
#	define XIO_PURE /*!*/
#	define XIO_PACKED /*!*/
#	if !defined(likely)
#		define likely(x) (x)
#	endif
#	if !defined(unlikely)
#		define unlikely(x) (x)
#	endif
#endif

#if defined(__GNUC__)
	#define GCC_VERSION(major, minor) ( \
		(__GNUC__ > (major)) || \
		(__GNUC__ == (major) && __GNUC_MINOR__ >= (minor)) \
	)
	#define CC_SUPPORTS_LAMBDA GCC_VERSION(4, 5)
	#define CC_SUPPORTS_RVALUE_REFERENCES GCC_VERSION(4, 4)
#else
	#define GCC_VERSION(major, minor) (0)
	#define CC_SUPPORTS_LAMBDA (0)
	#define CC_SUPPORTS_RVALUE_REFERENCES (0)
#endif

/// the filename only part of __FILE__ (no leading path)
#define __FILENAME__ ((std::strrchr(__FILE__, '/') ?: __FILE__ - 1) + 1)

#ifndef XZERO_NDEBUG
#	include <string>
#	include <ctime>
#	include <ev.h>
#	define DEBUG(msg...) do {                               \
		std::printf("%0.6f: ", ev_now(ev_default_loop(0))); \
		std::printf(msg); std::printf("\n");                \
	} while (false)
#else
#	define DEBUG(msg...) /*!*/
#endif
