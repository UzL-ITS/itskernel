#pragma once

/*
ITS kernel standard library I/O interface.
Provides functions for
	- Creating and managing a terminal
	- Printing formatted outputs (printf, sprintf, snprintf)
	- Reading keyboard inputs
*/

/* INCLUDES */

#include <stdbool.h>
#include <internal/keyboard/keycodes.h>

// printf, sprintf, snprintf
#include <internal/formatting/printf.h>


/* DECLARATIONS */

// Initializes the I/O interface and creates a terminal with the given amount of lines.
// Is called internally by the _start function.
void io_init(int lines);

// Reads a line from keyboard input.
// This function allocates a new char array that must be freed by the user.
char *getline();