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
	
	// Start LWIP thread
	printf("Starting LWIP thread...\n");
	run_thread(&itslwip_run, 0);
	printf("Thread started.\n");
	while(true) {}
	
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
		else if(strncmp(command, "test", 4) == 0)
		{
			uint8_t mac[6];
			sys_get_network_mac_address(mac);
			printf("MAC: %02x:%02x:%02x:%02x:%02x:%02x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
			
			uint8_t packet[] =
			{
				0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
				mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
				0x08, 0x06,
				
				0x00, 0x01,
				0x08, 0x00,
				0x06,
				0x04,
				0x00, 0x01,
				mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
				0xC0, 0xA8, 0x0A, 0x0A,
				0xFF, 0xFF, 0xFF, 0xFF,	0xFF, 0xFF,
				0xC0, 0xA8, 0x01, 0xFE
			};
			sys_send_network_packet(packet, sizeof(packet));
			printf("Packet sent.\n");
		}
		else
			printf("Unknown command.\n");
		
		// Free command string
		free(command);
	}
		
	// Exit with return code
	printf("Exiting...\n");
	_end(0);
}