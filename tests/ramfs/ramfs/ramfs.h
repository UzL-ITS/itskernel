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

	// Returned when a file is already opened.
	RAMFS_ERR_FILE_ALREADY_OPEN,

	// Returned when too many files are opened.
	RAMFS_ERR_TOO_MANY_OPEN_FILES,

} ramfs_err_t;

// Pointer to an opened file.
typedef int ramfs_fd_t;
#define RAMFS_FD_INVALID -1


/* DECLARATIONS */

// Initializes the RAM file system.
void ramfs_init();

// Opens the given file and stores the descriptor in the given variable.
// If the file does not exist, it is created.
ramfs_err_t ramfs_open(const char *path, ramfs_fd_t *fdPtr);

// Closes the given file.
void ramfs_close(ramfs_fd_t fd);

// Reads the given amount of bytes into the given buffer.
uint64_t ramfs_read(uint8_t *buffer, uint64_t length, ramfs_fd_t fd);

// Writes the given amount of bytes.
uint64_t ramfs_write(uint8_t *buffer, uint64_t length, ramfs_fd_t fd);

// Returns the current position in the file.
uint64_t ramfs_tell(ramfs_fd_t fd);

// Moves to the given position in the file.
void ramfs_seek(uint64_t position, ramfs_fd_t fd);

// Creates a new directory at the given path.
ramfs_err_t ramfs_create_directory(const char *path, const char *name);

// Retrieves a list of the contents of the given directory.
// Note: This function does NOT append a terminating 0-byte.
int ramfs_list(const char *path, char *buffer, int bufferLength);