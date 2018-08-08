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


/* TYPES */

// Function signature of a key press handler.
typedef void (*keypress_handler_t)(vkey_t keyCode, bool shiftPressed);


/* DECLARATIONS */

// Initializes the I/O interface and creates a terminal with the given amount of lines.
// Must be called by the user, before any input/output is performed.
void io_init(int lines);

// Reads a line from keyboard input.
char *getline();

// Registers a key press handler for the given key code.
// If there is already a handler present, false is returned.
bool register_keypress_handler(vkey_t keyCode, keypress_handler_t handler);

// Unregisters the key press handler for the given key code.
void unregister_keypress_handler(vkey_t keyCode);