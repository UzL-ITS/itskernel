/*
ITS kernel RAM file system implementation.
File and directory names are limited to 64 characters.
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
} ramfs_err_t;


/* DECLARATIONS */

// Initializes the RAM file system.
void ramfs_init();

// Creates a new directory under the given path.
ramfs_err_t ramfs_create_directory(const char *path, const char *name);

// Creates a new file at the given path.
ramfs_err_t ramfs_create_file(const char *path, const char *name, void *data, int dataLength);

// Returns the contents of the given file.
ramfs_err_t ramfs_get_file(const char *path, void *dataBuffer, int dataBufferLength);

// Returns the metadata of the given file.
ramfs_err_t ramfs_get_file_info(const char *path, int *dataLengthPtr);

// Dumps the entire RAM file system tree into the given string buffer.
void ramfs_dump(char *buffer, int bufferLength);

// Returns the size of the string buffer that is needed to write the entire tree.
int ramfs_dump_get_buffer_size();