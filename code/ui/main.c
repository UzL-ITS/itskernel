/*
Kernel UI process main file.
*/

/* INCLUDES */

#include <io.h>
#include <internal/syscall/syscalls.h>


/* FUNCTIONS */

// Handles a Fn key press.
static void handle_window_switch(vkey_t keyCode, bool shiftPressed)
{
	// Switch to given render context
	sys_set_displayed_process(keyCode - VKEY_F1);
}

int main()
{
	// Install handler for render context switch
	for(int c = VKEY_F1; c <= VKEY_F12; ++c)
		register_keypress_handler(c, &handle_window_switch);
	
	// test
	getline();
	
	return 0;
}