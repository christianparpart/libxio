/* <x0/FileInfoService.cpp>
 *
 * This file is part of the x0 web server project and is released under LGPL-3.
 * http://www.xzero.io/
 *
 * (c) 2009-2013 Christian Parpart <trapni@gmail.com>
 */

#include <x0/io/FileInfoService.h>
#include <x0/Tokenizer.h>
#include <x0/strutils.h>
#include <x0/sysconfig.h>

namespace x0 {

#if 0 // !defined(XZERO_NDEBUG)
#	define TRACE(msg...) printf("FileInfoService: " msg)
#else
#	define TRACE(msg...) /*!*/
#endif

#if defined(HAVE_SYS_INOTIFY_H)
#	undef HAVE_SYS_INOTIFY_H
#endif

FileInfoService::FileInfoService(struct ::ev_loop *loop, const Config *config) :
	loop_(loop),
#if 1 // defined(HAVE_SYS_INOTIFY_H)
	handle_(-1),
	inotify_(loop_),
	inotifies_(),
#endif
	config_(config),
	cache_()
{
#if defined(HAVE_SYS_INOTIFY_H)
	handle_ = inotify_init();
	if (handle_ != -1) {
		if (fcntl(handle_, F_SETFL, fcntl(handle_, F_GETFL) | O_NONBLOCK) < 0)
			fprintf(stderr, "Error setting nonblock/cloexec flags on inotify handle\n");

		if (fcntl(handle_, F_SETFD, fcntl(handle_, F_GETFD) | FD_CLOEXEC) < 0)
			fprintf(stderr, "Error setting cloexec flags on inotify handle\n");

		inotify_.set<FileInfoService, &FileInfoService::onFileChanged>(this);
		inotify_.start(handle_, ev::READ);
	} else {
		fprintf(stderr, "Error initializing inotify: %s\n", strerror(errno));
	}
#endif
}

FileInfoService::~FileInfoService()
{
#if defined(HAVE_SYS_INOTIFY_H)
	if (handle_ != -1) {
		::close(handle_);
	}
#endif
}

inline bool FileInfoService::isValid(const FileInfo *fi) const
{
	return fi->inotifyId_ > 0
		|| fi->cachedAt() + config_->cacheTTL_ > ev_now(loop_);
}

FileInfoPtr FileInfoService::query(const std::string& _filename)
{
	std::string filename(_filename[_filename.size() - 1] == '/'
			? _filename.substr(0, _filename.size() - 1)
			: _filename);

	auto i = cache_.find(filename);
	if (i != cache_.end()) {
		FileInfoPtr fi = i->second;
		if (isValid(fi.get())) {
			TRACE("query.cached(%s) len:%ld\n", filename.c_str(), fi->size());
			return fi;
		}

		TRACE("query.expired(%s) len:%ld\n", filename.c_str(), fi->size());
#if defined(HAVE_SYS_INOTIFY_H)
		if (fi->inotifyId_ >= 0) {
			inotify_rm_watch(handle_, fi->inotifyId_);
			auto i = inotifies_.find(fi->inotifyId_);
			if (i != inotifies_.end())
				inotifies_.erase(i);
		}
#endif
		cache_.erase(i);
	}

	if (FileInfoPtr fi = FileInfoPtr(new FileInfo(*this, filename))) {
		fi->mimetype_ = get_mimetype(filename);
		fi->etag_ = make_etag(*fi);

#if defined(HAVE_SYS_INOTIFY_H)
		int wd = handle_ != -1 && fi->exists()
				? ::inotify_add_watch(handle_, filename.c_str(),
					/*IN_ONESHOT |*/ IN_ATTRIB | IN_DELETE_SELF | IN_MOVE_SELF | IN_UNMOUNT |
					IN_DELETE_SELF | IN_MOVE_SELF)
				: -1;
		TRACE("query(%s).new -> %d len:%ld\n", filename.c_str(), wd, fi->size());

		if (wd != -1) {
			fi->inotifyId_ = wd;
			inotifies_[wd] = fi;
		}
		cache_[filename] = fi;
#else
		TRACE("query(%s)! len:%ld\n", filename.c_str(), fi->size());
		cache_[filename] = fi;
#endif

		return fi;
	}

	TRACE("query(%s) failed (%s)\n", filename.c_str(), strerror(errno));
	// either ::stat() or caching failed.

	return FileInfoPtr();
}


#if defined(HAVE_SYS_INOTIFY_H)
void FileInfoService::onFileChanged(ev::io& w, int revents)
{
	TRACE("onFileChanged()\n");

	char buf[sizeof(inotify_event) * 256];
	ssize_t rv = ::read(handle_, &buf, sizeof(buf));
	TRACE("read returned: %ld (%% %ld, %ld)\n",
			rv, sizeof(inotify_event), rv / sizeof(inotify_event));

	if (rv > 0) {
		const char *i = buf;
		const char *e = i + rv;
		inotify_event *ev = (inotify_event *)i;

		for (; i < e && ev->wd != 0; i += sizeof(*ev) + ev->len, ev = (inotify_event *)i) {
			TRACE("traverse: (wd:%d, mask:0x%04x, cookie:%d)\n", ev->wd, ev->mask, ev->cookie);
			auto wi = inotifies_.find(ev->wd);
			if (wi == inotifies_.end()) {
				TRACE("-skipping\n");
				continue;
			}

			auto k = cache_.find(wi->second->filename());
			TRACE("invalidate: %s\n", k->first.c_str());
			// onInvalidate(k->first, k->second);
			cache_.erase(k);
			inotifies_.erase(wi);
			int rv = inotify_rm_watch(handle_, ev->wd);
			if (rv < 0) {
				TRACE("error removing inotify watch (%d, %s): %s\n", ev->wd, ev->name, strerror(errno));
			} else {
				TRACE("inotify_rm_watch: %d (ok)\n", rv);
			}
		}
	}
}
#endif

void FileInfoService::Config::loadMimetypes(const std::string& filename)
{
	Buffer input(x0::read_file(filename));
	auto lines = Tokenizer<BufferRef, Buffer>::tokenize(input, "\n");

	mimetypes.clear();

	for (auto line: lines) {
		line = line.trim();
		auto columns = Tokenizer<BufferRef, BufferRef>::tokenize(line, " \t");

		auto ci = columns.begin(), ce = columns.end();
		BufferRef mime = ci != ce ? *ci++ : BufferRef();

		if (!mime.empty() && mime[0] != '#') {
			for (; ci != ce; ++ci) {
				mimetypes[ci->str()] = mime.str();
			}
		}
	}
}

std::string FileInfoService::get_mimetype(const std::string& filename) const
{
	std::size_t ndot = filename.find_last_of(".");
	std::size_t nslash = filename.find_last_of("/");

	if (ndot != std::string::npos && ndot > nslash)
	{
		std::string ext(filename.substr(ndot + 1));

		while (ext.size())
		{
			auto i = config_->mimetypes.find(ext);

			if (i != config_->mimetypes.end())
			{
				//DEBUG("filename(%s), ext(%s), use mimetype: %s", filename.c_str(), ext.c_str(), i->second.c_str());
				return i->second;
			}

			if (ext[ext.size() - 1] != '~')
				break;

			ext.resize(ext.size() - 1);
		}
	}

	//DEBUG("file(%s) use default mimetype", filename.c_str());
	return config_->defaultMimetype;
}

} // namespace x0
