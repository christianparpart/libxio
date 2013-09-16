#pragma once

#include <xio/Api.h>
#include <xio/DateTime.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <memory>
#include <ev++.h>

namespace xio {

class FileService;
class Stream;

class XIO_API File
{
public:
	explicit File(const std::string& path);
	~File();

	static void init(ev::loop_ref loop);
	static void deinit();

	const char* path() const { return path_.c_str(); }
	const char* filename() const;

	int error() const { return errno_; }
	bool exists() const { return errno_ == 0; }

	bool isDirectory() const { return S_ISDIR(stat_.st_mode); }
	bool isRegular() const { return S_ISREG(stat_.st_mode); }
	bool isExecutable() const { return stat_.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH); }

	std::size_t size() const { return stat_.st_size; }
	time_t mtime() const { return stat_.st_mtime; }

	const std::string& etag() const;
	const std::string& lastModified() const;
	const std::string& mimetype() const;

	bool updateCache();
	void clearCache();

	std::unique_ptr<Stream> source(int flags = O_RDONLY);
	std::unique_ptr<Stream> sink(int flags = O_WRONLY);

	const struct stat* operator->() const { return &stat_; }

private:
	std::string path_;
	struct stat stat_;
	int errno_;

	int inotify_;
	DateTime cachedAt_;

	mutable std::string etag_;
	mutable std::string mtime_;
	mutable std::string mimetype_;
};

typedef std::shared_ptr<File> FilePtr;

} // namespace xio
