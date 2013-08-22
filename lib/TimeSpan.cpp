/* <x0/TimeSpan.cpp>
 *
 * This file is part of the x0 web server project and is released under LGPL-3.
 * http://www.xzero.io/
 *
 * (c) 2009-2013 Christian Parpart <trapni@gmail.com>
 */

#include <xio/TimeSpan.h>

const TimeSpan TimeSpan::Zero(static_cast<size_t>(0));

#if 0
#include <xio/Buffer.h>
#include <string>
#include <cstdint>

std::string TimeSpan::str() const
{
	int totalMinutes =
		days() * 24 * 60 +
		hours() * 60 +
		minutes();

	Buffer b(64);
	b << totalMinutes << "m " << seconds() << "s";
	return b.str();
}
#endif
