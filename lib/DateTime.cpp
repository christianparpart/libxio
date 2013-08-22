#include <xio/DateTime.h>

DateTime::DateTime() :
	value_(std::time(0))
{
}

DateTime::DateTime(const std::string& v) :
	value_(mktime(v.c_str()))
{
}

DateTime::DateTime(ev::tstamp v) :
	value_(v)
{
}

DateTime::~DateTime()
{
}
