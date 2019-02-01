#pragma once
/*
ITS kernel RAM file system types.
*/

/* INCLUDES */

#include <stdint.h>


/* TYPES */

// The different error codes returned by the ramfs functions.
typedef enum
{
	// No error occured.
	RAMFS_ERR_OK,

	// Returned when an invalid path string is passed.
	RAMFS_ERR_INVALID_PATH_FORMAT,

	// Returned when an illegal character (e.g. slash in a file name) is encountered.
	RAMFS_ERR_ILLEGAL_CHARACTER,

	// Returned when the given path contains a non-existing directory.
	RAMFS_ERR_DIRECTORY_DOES_NOT_EXIST,

	// Returned when a directory to be created already exists.
	RAMFS_ERR_DIRECTORY_EXISTS,

	// Returned when the given file does not exist.
	RAMFS_ERR_FILE_DOES_NOT_EXIST,

	// Returned when a file to be created already exists.
	RAMFS_ERR_FILE_EXISTS,

	// Returned when a provided buffer is too small.
	RAMFS_ERR_BUFFER_TOO_SMALL,

	// Returned when a file is already opened.
	RAMFS_ERR_FILE_ALREADY_OPEN,

	// Returned when too many files are opened.
	RAMFS_ERR_TOO_MANY_OPEN_FILES,

} ramfs_err_t;

// The different seek starting positions.
typedef enum
{
    // Seek starting at the file begin.
    RAMFS_SEEK_START,

    // Seek starting at current position.
    RAMFS_SEEK_CURRENT,

    // Seek starting at file end.
    RAMFS_SEEK_END

} ramfs_seek_whence_t;

// Pointer to an opened file.
typedef int ramfs_fd_t;
#define RAMFS_FD_INVALID -1