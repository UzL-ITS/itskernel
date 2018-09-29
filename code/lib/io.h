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

// File system error type.
typedef ramfs_err_t fs_err_t;


/* DECLARATIONS */

// Initializes the I/O interface and creates a terminal with the given amount of lines.
// Is called internally by the _start function.
void io_init(int lines);

// Reads a line from keyboard input.
// This function allocates a new char array that must be freed by the user.
char *getline();

// Creates a new directory under the given path.
fs_err_t create_directory(const char *path, const char *name);

// Creates a new file at the given path.
fs_err_t create_file(const char *path, const char *name, void *data, int dataLength);

// Returns the contents of the given file.
fs_err_t get_file(const char *path, void **dataPtr, int *dataLengthPtr);

// Dumps the entire RAM file system tree to terminal.
void dump_files();