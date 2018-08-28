/*
Kernel UI process main file.
*/

/* INCLUDES */

#include <app.h>
#include <io.h>
#include <internal/syscall/syscalls.h>
#include <threading/thread.h>
#include <internal/keyboard/keyboard.h>
#include <memory.h>
#include <stdbool.h>
#include <string.h>


/* FUNCTIONS */

// Handles a Fn key press.
static void handle_window_switch(vkey_t keyCode, bool shiftPressed)
{
	// Switch to given render context
	sys_set_displayed_process(keyCode - VKEY_F1);
}

void main()
{
	// Initialize library
	_start();
	
	// Banner
	printf("--- ITS Micro Kernel :: UI PROCESS ---\n");
	
	// Install handler for render context switch
	printf("Installing context switch handlers...");
	for(int c = VKEY_F1; c <= VKEY_F12; ++c)
		register_keypress_handler(c, &handle_window_switch);
	printf("OK\n");
	
	// Command loop
	while(true)
	{
		// Print command line string
		printf("\nitskernel$ ");
		char *command = getline();
		int commandLength = strlen(command);
		
		// Handle command
		if(strncmp(command, "exit", 4) == 0)
			break;
		else
			printf("Unknown command.\n");
		
		// Free command string
		free(command);
	}
		
	// Exit with return code
	printf("Exiting...\n");
	_end(0);
}