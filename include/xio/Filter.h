#pragma once
/* <Filter.h>
 *
 * This file is part of the xio web server project and is released under LGPL-3.
 * http://www.xzero.io/
 *
 * (c) 2009-2013 Christian Parpart <trapni@gmail.com>
 */

#include <xio/Buffer.h>
#include <xio/Api.h>

namespace xio {

//! \addtogroup io
//@{

/** Unidirectional data processor.
 *
 * A filter is a processor, that reads from a source, and passes 
 * the received data to the sink. this data may or may not be
 * transformed befor passing to the sink.
 *
 * \see FilterStream
 */
class XIO_API Filter
{
public:
	virtual ~Filter() {}

	/** 
	 * Processes given input data through this filter.
	 *
	 * @param input input buffer to process on
	 * @param output output buffer to append the processed data into.
	 */
	virtual ssize_t process(const BufferRef& input, Buffer& output) = 0;
};

//@}

} // namespace xio
