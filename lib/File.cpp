#include <xio/File.h>
#include <xio/FileStream.h>
#include <sys/inotify.h>
#include <ev++.h>

namespace xio {

class Inotify {
public:
	Inotify() :
		handle_(inotify_init1(IN_NONBLOCK | IN_CLOEXEC))
	{
	}

	~Inotify() {
		::close(handle_);
	}

	int add(const std::string& path, int mode) {
		return inotify_add_watch(handle_, path.c_str(), mode);
	}

	void remove(int wd) {
		inotify_rm_watch(handle_, wd);
	}

	int handle() const { return handle_; }

	void init(ev::loop_ref loop)
	{
		io_.reset(new ev::io(loop));
		io_->set<Inotify, &Inotify::callback>(this);
		io_->set(handle_, ev::READ);
		io_->start();
	}

	void deinit()
	{
		io_->stop();
		io_.reset(nullptr);
	}

	void callback(ev::io& io, int revents)
	{
		// TODO
	}

	void dispatch()
	{
		// TODO
	}

private:
	int handle_;
	std::unique_ptr<ev::io> io_;
};

/*thread_local*/ Inotify inotifier;

void File::init(ev::loop_ref loop)
{
	inotifier.init(loop);
}

void File::deinit()
{
	inotifier.deinit();
}

File::File(const std::string& path) :
	path_(path),
	stat_(),
	errno_(),
	inotify_(-1),
	cachedAt_(),
	etag_(),
	mtime_(),
	mimetype_()
{
}

File::~File()
{
	if (inotify_ != -1) {
		inotifier.remove(inotify_);
		inotify_ = -1;
	}
}

const char* File::filename() const
{
	return path_.c_str(); // FIXME
}

const std::string& File::etag() const
{
	return etag_;
}

const std::string& File::lastModified() const
{
	return mtime_;
}

const std::string& File::mimetype() const
{
	return mimetype_;
}

bool File::updateCache()
{
	if (inotify_ < 0)
		inotifier.remove(inotify_);

	inotify_ = inotifier.add(path_, IN_MODIFY | IN_MOVE_SELF | IN_MOVED_FROM | IN_MOVED_TO);

	return false;
}

void File::clearCache()
{
	if (inotify_ != -1) {
		inotifier.remove(inotify_);
		inotify_ = -1;
	}
}

std::unique_ptr<FileStream> File::open(int flags)
{
	int fd = ::open(path_.c_str(), flags);
	if (fd < 0)
		return std::unique_ptr<FileStream>();

	return std::unique_ptr<FileStream>(new FileStream(fd));
}

} // namespace xio
