/*
ITS kernel standard library I/O interface.
*/

/* INCLUDES */

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <internal/keyboard/keyboard.h>
#include <internal/terminal/terminal.h>

#include <internal/syscall/syscalls.h>


/* VARIABLES */

// Default terminal line count.
static const int defaultLineCount = 1000;

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

void printf(const char *format, ...)
{
	// Initialized?
	if(!initDone)
		io_init(defaultLineCount);
	
	// Get variadic argument list
	va_list args;
	va_start(args, format);
	
	// TODO
	terminal_puts(format);
	
	// Free variadic argument list
	va_end(args);
}

char *getline()
{
	// Initialized?
	if(!initDone)
		io_init(defaultLineCount);
	
	while(true)
	{
		bool shiftPressed;
		vkey_t key = receive_keypress(&shiftPressed);
		char buf[24] = "Printable character:  \n\0";
		if(key == VKEY_ENTER)
			sys_kputs("LIB: ENTER key!\n");
		else if(key_is_navigation_key(key))
			terminal_handle_navigation_key(key);
		else if(key_is_printable_character(key))
		{
			buf[21] = key_to_character(key, shiftPressed);
			sys_kputs(buf);
		}
		else if(keypressHandlers[key] != 0)
		{
			sys_kputs("LIB: Key handler called.\n");
			keypressHandlers[key](key, shiftPressed);
		}
		else
			sys_kputs("LIB: Ignored key.\n");
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