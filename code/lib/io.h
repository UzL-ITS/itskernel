#pragma once

/*
ITS kernel standard library I/O interface.
Provides functions for
	- Creating and managing a terminal
	- Printing formatted outputs (printf, sprintf, snprintf)
	- Reading keyboard inputs
	- Creating and reading files and directories
*/

/* INCLUDES */

#include <stdbool.h>
#include <internal/keyboard/keycodes.h>
#include <internal/syscall/ramfs.h>

// printf, sprintf, snprintf
#include <internal/formatting/printf.h>


/* TYPES */

// The different error codes returned by the ramfs functions.
typedef enum
{
	// No error occured.
	FS_ERR_OK = RAMFS_ERR_OK,

	// Returned when an invalid path string is passed.
	FS_ERR_INVALID_PATH_FORMAT = RAMFS_ERR_INVALID_PATH_FORMAT,

	// Returned when an illegal character (e.g. slash in a file name) is encountered.
	FS_ERR_ILLEGAL_CHARACTER = RAMFS_ERR_ILLEGAL_CHARACTER,

	// Returned when the given path contains a non-existing directory.
	FS_ERR_DIRECTORY_DOES_NOT_EXIST = RAMFS_ERR_DIRECTORY_DOES_NOT_EXIST,

	// Returned when a directory to be created already exists.
	FS_ERR_DIRECTORY_EXISTS = RAMFS_ERR_DIRECTORY_EXISTS,

	// Returned when the given file does not exist.
	FS_ERR_FILE_DOES_NOT_EXIST = RAMFS_ERR_FILE_DOES_NOT_EXIST,

	// Returned when a file to be created already exists.
	FS_ERR_FILE_EXISTS = RAMFS_ERR_FILE_EXISTS,

	// Returned when a provided buffer is too small.
	FS_ERR_BUFFER_TOO_SMALL = RAMFS_ERR_BUFFER_TOO_SMALL,

	// Returned when a file is already opened.
	FS_ERR_FILE_ALREADY_OPEN = RAMFS_ERR_FILE_ALREADY_OPEN,

	// Returned when too many files are opened.
	FS_ERR_TOO_MANY_OPEN_FILES = RAMFS_ERR_TOO_MANY_OPEN_FILES,

} fs_err_t;

// The different starting positions of the ramfs seek function.
typedef enum
{
    // Seek starting at the file begin.
    FS_SEEK_START = RAMFS_SEEK_START,

    // Seek starting at current position.
    FS_SEEK_CURRENT = RAMFS_SEEK_CURRENT,

    // Seek starting at file end.
    FS_SEEK_END = RAMFS_SEEK_END

} fs_seek_whence_t;

// Pointer to an opened file.
typedef ramfs_fd_t fs_fd_t;


/* DECLARATIONS */

// Initializes the I/O interface and creates a terminal with the given amount of lines.
// Is called internally by the _start function.
void io_init(int lines);

// Reads a line from keyboard input.
// This function allocates a new char array that must be freed by the user.
char *getline();

// Opens the given file and stores the descriptor in the given variable.
// If the file does not exist and the "create" flag is set, it is created.
fs_err_t fopen(const char *path, fs_fd_t *fdPtr, bool create);

// Closes the given file.
void fclose(fs_fd_t fd);

// Reads the given amount of bytes into the given buffer.
uint64_t fread(uint8_t *buffer, uint64_t length, fs_fd_t fd);

// Writes the given amount of bytes.
uint64_t fwrite(uint8_t *buffer, uint64_t length, fs_fd_t fd);

// Returns the current position in the file.
uint64_t ftell(fs_fd_t fd);

// Moves to the given position in the file.
void fseek(int64_t offset, fs_seek_whence_t whence, fs_fd_t fd);

// Creates a new directory at the given path.
fs_err_t create_directory(const char *path, const char *name);

// Tries to access the directory at the given path and returns suitable error codes (FS_ERR_DIRECTORY_EXISTS vs. FS_ERR_DIRECTORY_DOES_NOT_EXIST).
fs_err_t test_directory(const char *path, const char *name);

// Retrieves a list of the contents of the given directory.
// Note: This function does NOT append a terminating 0-byte.
int flist(const char *path, char *buffer, int bufferLength);

// Deletes the given file.
fs_err_t fdelete(const char *path);