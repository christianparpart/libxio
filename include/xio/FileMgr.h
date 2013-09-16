/* <FileMgr.h>
 *
 * This file is part of the x0 web server project and is released under LGPL-3.
 * http://www.xzero.io/
 *
 * (c) 2009-2013 Christian Parpart <trapni@gmail.com>
 */

#ifndef sw_x0_FileMgr_h
#define sw_x0_FileMgr_h (1)

#include <xio/Api.h>
#include <xio/sysconfig.h>
#include <string>
#include <sstream>
#include <unordered_map>

#include <ev++.h>

#if defined(HAVE_SYS_INOTIFY_H)
#	include <sys/inotify.h>
#	include <sys/fcntl.h>
#endif

namespace xio {

//! \addtogroup io
//@{

/** service for retrieving file information.
 *
 * This is like stat(), in fact, it's using stat() and more magic, but
 * caches the result for further use and also invalidates in realtime the file-info items
 * in case their underlying inode has been updated.
 *
 * \note this class is not thread-safe
 */
class X0_API FileMgr
{
public:
	struct Config // {{{
	{
		bool etagConsiderMtime;							//!< flag, specifying wether or not the file modification-time is part of the ETag
		bool etagConsiderSize;							//!< flag, specifying wether or not the file size is part of the ETag
		bool etagConsiderInode;							//!< flag, specifying wether or not the file inode number is part of the ETag

		std::unordered_map<std::string, std::string> mimetypes;	//!< cached database for file extension to mimetype mapping
		std::string defaultMimetype;					//!< default mimetype for those files we could not determine the mimetype.

		int cacheTTL_;									//!< time in seconds to keep File-object in-cache.

		Config() :
			etagConsiderMtime(true),
			etagConsiderSize(true),
			etagConsiderInode(false),
			mimetypes(),
			defaultMimetype("text/plain"),
			cacheTTL_(10)
		{}

		void loadMimetypes(const std::string& filename);
	}; // }}}

private:
	struct ::ev_loop *loop_;

#if defined(HAVE_SYS_INOTIFY_H)
	int handle_;									//!< inotify handle
	ev::io inotify_;
	std::unordered_map<int, FilePtr> inotifies_;
#endif

	const Config *config_;
	std::unordered_map<std::string, FilePtr> cache_;		//!< cache, storing path->File pairs

public:
	FileMgr(struct ::ev_loop *loop, const Config *config);
	~FileMgr();

	FileMgr(const FileMgr&) = delete;
	FileMgr& operator=(const FileMgr&) = delete;

	FilePtr query(const std::string& filename);
	FilePtr operator()(const std::string& filename);

	std::size_t size() const;
	bool empty() const;

private:
	friend class File;

	inline bool isValid(const File *finfo) const;

	std::string get_mimetype(const std::string& ext) const;
	std::string make_etag(const File& fi) const;
	void onFileChanged(ev::io& w, int revents);
};

} // namespace xio

//@}

// {{{ implementation
#include <xio/File.h>

namespace xio {

inline FilePtr FileMgr::operator()(const std::string& filename)
{
	return query(filename);
}

inline std::size_t FileMgr::size() const
{
	return cache_.size();
}

inline bool FileMgr::empty() const
{
	return cache_.empty();
}

inline std::string FileMgr::make_etag(const File& fi) const
{
	int count = 0;
	std::stringstream sstr;

	sstr << '"';

	// TODO encode numbers in hex than dec (should be a tick faster)

	if (config_->etagConsiderMtime) {
		if (count++) sstr << '-';
		sstr << fi->st_mtime;
	}

	if (config_->etagConsiderSize) {
		if (count++) sstr << '-';
		sstr << fi->st_size;
	}

	if (config_->etagConsiderInode) {
		if (count++) sstr << '-';
		sstr << fi->st_ino;
	}

	/// \todo support checksum etags (crc, md5, sha1, ...) - although, btrfs supports checksums directly on filesystem level!

	sstr << '"';

	return sstr.str();
}

} // namespace xio

// }}}

#endif
