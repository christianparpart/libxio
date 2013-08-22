#pragma once
/* <x0/Api.h>
 *
 * This file is part of the x0 web server project and is released under LGPL-3.
 * http://www.xzero.io/
 *
 * (c) 2009-2013 Christian Parpart <trapni@gmail.com>
 */

#include <xio/Defines.h>

// xio lib exports
#if defined(BUILD_XIO)
#	define XIO_API XIO_EXPORT
#else
#	define XIO_API XIO_IMPORT
#endif
