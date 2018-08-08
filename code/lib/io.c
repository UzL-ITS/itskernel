/*
ITS kernel standard library I/O interface.
*/

/* INCLUDES */

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <internal/keyboard/keyboard.h>
#include <internal/terminal/terminal.h>
#include <string.h>


/* VARIABLES */

// Determines whether initialization has completed.
static bool initDone = false;

// The keypress handler for each key code.
static keypress_handler_t keypressHandlers[VKEY_MAX_VALUE + 1] = { 0 };


/* FUNCTIONS */

void io_init(int lines)
{
	// Only initialize once
	if(initDone)
		return;
	
	// Initialize terminal
	terminal_init(lines);
	
	// Initialized
	initDone = true;
}

char *getline()
{
	// Initialized?
	if(!initDone)
		return 0;
	
	while(true)
	{
		bool shiftPressed;
		vkey_t key = receive_keypress(&shiftPressed);
		if(key_is_navigation_key(key))
			terminal_handle_navigation_key(key);
		else if(key_is_printable_character(key))
		{
			// TODO
		}
		else if(keypressHandlers[key] != 0)
		{
			keypressHandlers[key](key, shiftPressed);
		}
	}
	return 0;
}

bool register_keypress_handler(vkey_t keyCode, keypress_handler_t handler)
{
	// Handler slot empty?
	if(keypressHandlers[keyCode] != 0)
		return false;
	
	// Install handler
	keypressHandlers[keyCode] = handler;
	return true;
}

void unregister_keypress_handler(vkey_t keyCode)
{
	// Uninstall handler
	keypressHandlers[keyCode] = 0;
}