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
#include <fs/ramfs.h>
#include "net/itslwip/itslwip.h"


/* FUNCTIONS */

// Handles a Fn key press.
static void handle_window_switch(vkey_t keyCode, bool shiftPressed)
{
	// Switch to given render context
	sys_set_displayed_process(keyCode - VKEY_F1);
}

// Splits the given command string into arguments and provides an array pointing to the different parts.
// The original string is modified!
static char **split_command_string(char *command, int *argumentCount)
{
	// We don't handle more than 10 arguments
	char **args = (char **)malloc(10 * sizeof(char *));
	for(int i = 0; i < 10; ++i)
	{
		// Update argument counter
		*argumentCount = i;
		
		// Turn whitespace into terminating zeroes, such that args[i] can be used as single strings
		while(*command == ' ')
		{
			*command = '\0';
			++command;
		}
		if(*command == '\0')
			return args;
		
		// Save pointer to next argument
		args[i] = command;
		
		// Skip argument content
		while(*command != ' ')
			if(*command++ == '\0')
				return args;
	}
	return args;
}

void main()
{
	// Initialize library
	_start();
	
	// Initialize file system
	ramfs_init();
	
	// Banner
	printf_locked("--- ITS Micro Kernel :: UI PROCESS ---\n");
	
	// Install handler for render context switch
	printf_locked("Installing context switch handlers...");
	for(int c = VKEY_F1; c <= VKEY_F12; ++c)
		register_keypress_handler(c, &handle_window_switch);
	printf_locked("OK\n");
	
	// Create file system
	printf_locked("Creating RAM file system...");
	if(ramfs_create_directory("/", "in") != RAMFS_ERR_OK || ramfs_create_directory("/", "out") != RAMFS_ERR_OK)
		printf_locked("failed\n");
	else
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
		int argCount;
		char **args = split_command_string(command, &argCount);
		if(argCount < 1)
		{
			printf_locked("Please type a command.\n");
			continue;
		}
		
		// Handle command
		if(strcmp(args[0], "exit") == 0)
			break;
		else if(strcmp(args[0], "lss") == 0)
		{
			// Connect to server and send command
			tcp_handle_t tcpHandle = itslwip_connect("192.168.20.1", 17571);
			itslwip_send_string(tcpHandle, "ls\n", 3);
			
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
			itslwip_send_string(tcpHandle, "exit\n", 5);
			itslwip_disconnect(tcpHandle);
		}
		else if(strcmp(args[0], "ls") == 0 || strcmp(args[0], "ll") == 0)
		{
			// Dump entire file system tree
			ramfs_dump();
		}
		else if(strcmp(args[0], "dl") == 0)
		{
			// Get file name
			if(argCount < 2)
			{
				printf_locked("Missing argument.\n");
				break;
			}
			int filenameLength = strlen(args[1]);
			char *filename = (char *)malloc(filenameLength + 1);
			strncpy(filename, args[1], filenameLength);
			filename[filenameLength] = '\n';
			
			// Connect to server and send command
			tcp_handle_t tcpHandle = itslwip_connect("192.168.20.1", 17571);
			itslwip_send_string(tcpHandle, "sendin\n", 7);
			itslwip_send_string(tcpHandle, filename, filenameLength + 1);
			
			// Receive file size
			itslwip_receive_line(tcpHandle, lineBuffer, sizeof(lineBuffer));
			int fileSize = atoi(lineBuffer);
			printf_locked("File size: %d bytes\n", fileSize);
			
			// Receive file data
			uint8_t *fileData = (uint8_t *)malloc(fileSize);
			itslwip_receive_data(tcpHandle, fileData, fileSize);
			
			// Store file
			if(ramfs_create_file("/in", args[1], fileData, fileSize) != RAMFS_ERR_OK)
				printf_locked("Error saving received file.\n");
			
			// Disconnect
			itslwip_send_string(tcpHandle, "exit\n", 5);
			itslwip_disconnect(tcpHandle);
			free(filename);
			free(args);
		}
		else if(strcmp(args[0], "prefix") == 0)
		{
			// Load file
			if(argCount < 3)
			{
				printf_locked("Missing argument.\n");
				break;
			}
			uint8_t *fileData;
			int fileLength;
			if(ramfs_get_file(args[1], (void **)&fileData, &fileLength) == RAMFS_ERR_OK)
			{
				// Print first bytes
				int dumpLength = atoi(args[2]);
				if(dumpLength > fileLength)
					dumpLength = fileLength;
				for(int i = 0; i < dumpLength; ++i)
					printf_locked("%c", fileData[i]);
			}
			else
				printf_locked("File not found.\n");
		}
		else
			printf_locked("Unknown command.\n");
		
		// Free command string
		free(args);
		free(command);
	}
		
	// Exit with return code
	printf_locked("Exiting...\n");
	_end(0);
}