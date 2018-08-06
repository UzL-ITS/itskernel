#pragma once

/*
ITS kernel standard library I/O interface.
Provides functions for
	- Creating and managing a terminal
	- Printing outputs
	- Reading keyboard inputs
	- Parsing format strings
*/

/* INCLUDES */

#include <stdbool.h>
#include <internal/keyboard/keycodes.h>


/* TYPES */

// Function signature of a key press handler.
typedef void (*keypress_handler_t)(vkey_t keyCode, bool shiftPressed);


/* DECLARATIONS */

// Initializes the I/O interface and creates a terminal with the given amount of lines.
// If this function is not called by the user, it is called with default values once the first input/output is encountered.
void io_init(int lines);

// Formats and outputs a string. For an explanation have a look at the C documentation.
// Currently supported format strings:
//    
void printf(const char *format, ...);

// Reads a line from keyboard input.
char *getline();

// Registers a key press handler for the given key code.
// If there is already a handler present, false is returned.
bool register_keypress_handler(vkey_t keyCode, keypress_handler_t handler);

// Unregisters the key press handler for the given key code.
void unregister_keypress_handler(vkey_t keyCode);