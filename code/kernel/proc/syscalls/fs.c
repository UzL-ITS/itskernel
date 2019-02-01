
#include <proc/syscalls.h>

ramfs_err_t sys_fs_open(const char *path, ramfs_fd_t *fdPtr)
{
	return ramfs_open(path, fdPtr);
}

void sys_fs_close(ramfs_fd_t fd)
{
	ramfs_close(fd);
}

uint64_t sys_fs_read(uint8_t *buffer, uint64_t length, ramfs_fd_t fd)
{
	return ramfs_read(buffer, length, fd);
}

uint64_t sys_fs_write(uint8_t *buffer, uint64_t length, ramfs_fd_t fd)
{
	return ramfs_write(buffer, length, fd);
}

uint64_t sys_fs_tell(ramfs_fd_t fd)
{
	return ramfs_tell(fd);
}

void sys_fs_seek(int64_t offset, ramfs_seek_whence_t whence, ramfs_fd_t fd)
{
	ramfs_seek(offset, whence, fd);
}

ramfs_err_t sys_fs_create_directory(const char *path, const char *name)
{
	return ramfs_create_directory(path, name);
}

ramfs_err_t sys_fs_test_directory(const char *path, const char *name)
{
	return ramfs_test_directory(path, name);
}

int sys_fs_list(const char *path, char *buffer, int bufferLength)
{
	return ramfs_list(path, buffer, bufferLength);
}
