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
#include <stdint.h>
#include "net/itslwip/itslwip.h"


/* FUNCTIONS */

// Handles a Fn key press.
static void handle_window_switch(vkey_t keyCode, bool shiftPressed)
{
	// Switch to given render context
	sys_set_displayed_process(keyCode - VKEY_F1);
}

// Returns the next argument of the given command string.
static const char *next_command_argument(const char *command, int *argumentLength)
{
	// Skip current argument
	while(*command != ' ')
		if(*command++ == '\0')
			return 0;
	
	// Skip whitespace
	while(*command == ' ')
		if(*command++ == '\0')
			return 0;
	return command;
}

void main()
{
	// Initialize library
	_start();
	
	// Banner
	printf_locked("--- ITS Micro Kernel :: UI PROCESS ---\n");
	
	// Install handler for render context switch
	printf_locked("Installing context switch handlers...");
	for(int c = VKEY_F1; c <= VKEY_F12; ++c)
		register_keypress_handler(c, &handle_window_switch);
	printf_locked("OK\n");
	
	// Start LWIP thread
	printf_locked("Starting LWIP thread...\n");
	run_thread(&itslwip_run, 0);
	printf_locked("Thread started.\n");
	
	// Command loop
	char lineBuffer[1000];
	while(true)
	{
		// Print command line string
		printf_locked("\n~$ ");
		char *command = getline();
		int commandLength = strlen(command);
		
		// Handle command
		if(strncmp(command, "exit", 4) == 0)
			break;
		else if(strncmp(command, "lss", 3) == 0)
		{
			// Connect to server and send command
			tcp_handle_t tcpHandle = itslwip_connect("192.168.20.1", 17571);
			itslwip_send(tcpHandle, "ls\n", 3);
			
			// Receive file count
			itslwip_receive_line(tcpHandle, lineBuffer, sizeof(lineBuffer));
			int count = atoi(lineBuffer);
			printf_locked("Server has %d input files:\n", count);
			
			// Receive file list
			for(int i = 0; i < count; ++i)
			{
				itslwip_receive_line(tcpHandle, lineBuffer, sizeof(lineBuffer));
				printf_locked("    %s\n", lineBuffer);
			}
			
			// Disconnect
			itslwip_send(tcpHandle, "exit\n", 5);
			itslwip_disconnect(tcpHandle);
		}
		else if(strncmp(command, "dl ", 3) == 0)
		{
			// Get file name
			int arg1Length;
			char *arg1 = next_command_argument(command, &arg1Length);
			
			// TODO
		}
		else
			printf_locked("Unknown command.\n");
		
		// Free command string
		free(command);
	}
		
	// Exit with return code
	printf_locked("Exiting...\n");
	_end(0);
}