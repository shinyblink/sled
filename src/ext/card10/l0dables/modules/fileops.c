/**
 * Implemention of the epicardium epic_file_ api that operates on
 * the global EpicFileSystem instance
 *
 * All functions lock & unlock the global FS object
 *
 */

#include "fs/internal.h"

#if defined(__GNUC__) &&                                                       \
	((__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
#define HAVE_STATIC_ASSERT 1
#elif defined(__clang__)
#define HAVE_STATIC_ASSERT 1
#endif
#if HAVE_STATIC_ASSERT
_Static_assert(sizeof(struct epic_stat) == 276, "");
#endif

int epic_file_open(const char *filename, const char *mode)
{
	EpicFileSystem *fs;
	int res = efs_lock_global(&fs);
	if (res == 0) {
		res = efs_open(fs, filename, mode);
		efs_unlock_global(fs);
	}
	return res;
}

int epic_file_close(int fd)
{
	EpicFileSystem *fs;
	int res = efs_lock_global(&fs);
	if (res == 0) {
		res = efs_close(fs, fd);
		efs_unlock_global(fs);
	}
	return res;
}

int epic_file_read(int fd, void *buf, size_t nbytes)
{
	EpicFileSystem *fs;
	int res = efs_lock_global(&fs);
	if (res == 0) {
		res = efs_read(fs, fd, buf, nbytes);
		efs_unlock_global(fs);
	}
	return res;
}

int epic_file_write(int fd, const void *buf, size_t nbytes)
{
	EpicFileSystem *fs;
	int res = efs_lock_global(&fs);
	if (res == 0) {
		res = efs_write(fs, fd, buf, nbytes);
		efs_unlock_global(fs);
	}
	return res;
}

int epic_file_flush(int fd)
{
	EpicFileSystem *fs;
	int res = efs_lock_global(&fs);
	if (res == 0) {
		res = efs_flush(fs, fd);
		efs_unlock_global(fs);
	}
	return res;
}

int epic_file_seek(int fd, long offset, int whence)
{
	EpicFileSystem *fs;
	int res = efs_lock_global(&fs);
	if (res == 0) {
		res = efs_seek(fs, fd, offset, whence);
		efs_unlock_global(fs);
	}
	return res;
}

int epic_file_tell(int fd)
{
	EpicFileSystem *fs;
	int res = efs_lock_global(&fs);
	if (res == 0) {
		res = efs_tell(fs, fd);
		efs_unlock_global(fs);
	}
	return res;
}

int epic_file_stat(const char *filename, struct epic_stat *stat)
{
	EpicFileSystem *fs;
	int res = efs_lock_global(&fs);
	if (res == 0) {
		res = efs_stat(fs, filename, stat);
		efs_unlock_global(fs);
	}
	return res;
}

int epic_file_opendir(const char *path)
{
	EpicFileSystem *fs;
	int res = efs_lock_global(&fs);
	if (res == 0) {
		res = efs_opendir(fs, path);
		efs_unlock_global(fs);
	}
	return res;
}

int epic_file_readdir(int fd, struct epic_stat *stat)
{
	EpicFileSystem *fs;
	int res = efs_lock_global(&fs);
	if (res == 0) {
		res = efs_readdir(fs, fd, stat);
		efs_unlock_global(fs);
	}
	return res;
}

int epic_file_unlink(const char *path)
{
	EpicFileSystem *fs;
	int res = efs_lock_global(&fs);
	if (res == 0) {
		res = efs_unlink(fs, path);
		efs_unlock_global(fs);
	}
	return res;
}

int epic_file_rename(const char *oldp, const char *newp)
{
	EpicFileSystem *fs;
	int res = efs_lock_global(&fs);
	if (res == 0) {
		res = efs_rename(fs, oldp, newp);
		efs_unlock_global(fs);
	}
	return res;
}

int epic_file_mkdir(const char *dirname)
{
	EpicFileSystem *fs;
	int res = efs_lock_global(&fs);
	if (res == 0) {
		res = efs_mkdir(fs, dirname);
		efs_unlock_global(fs);
	}
	return res;
}
