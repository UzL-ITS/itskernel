
#include <proc/syscalls.h>

ramfs_err_t sys_create_directory(const char *path, const char *name)
{
	return ramfs_create_directory(path, name);
}

ramfs_err_t sys_create_file(const char *path, const char *name, void *data, int dataLength)
{
	return ramfs_create_file(path, name, data, dataLength);
}

ramfs_err_t sys_get_file(const char *path, void *dataBuffer, int dataBufferLength)
{
	return ramfs_get_file(path, dataBuffer, dataBufferLength);
}

ramfs_err_t sys_get_file_info(const char *path, int *dataLengthPtr)
{
	return ramfs_get_file_info(path, dataLengthPtr);
}

void sys_dump_files(char *buffer, int bufferLength)
{
	ramfs_dump(buffer, bufferLength);
}

int sys_dump_files_get_buffer_size()
{
	return ramfs_dump_get_buffer_size();
}