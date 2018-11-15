/*
Kernel UI process main file.
*/

/* INCLUDES */

#include <app.h>
#include <io.h>
#include <internal/syscall/syscalls.h>
#include <threading/thread.h>
#include <threading/process.h>
#include <internal/keyboard/keyboard.h>
#include <memory.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include "net/itslwip/itslwip.h"
#include <internal/terminal/terminal.h>


/* VARIABLES */

// Network configuration.
//*
static char serverIpAddress[] = "141.83.62.232";
static char ipAddress[] = "141.83.62.44";
static char subnetMask[] = "255.255.255.0";
static char gatewayAddress[] = "141.83.62.1";
/*
static char serverIpAddress[] = "192.168.21.1"; // VMware
static char ipAddress[] = "192.168.21.10";
static char subnetMask[] = "255.255.255.0";
static char gatewayAddress[] = "192.168.21.1";
/*
static char serverIpAddress[] = "192.168.20.1"; // VirtualBox
static char ipAddress[] = "192.168.20.10";
static char subnetMask[] = "255.255.255.0";
static char gatewayAddress[] = "192.168.20.1";
/**/

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
	*argumentCount = 0;
	for(int i = 0; i < 10; ++i)
	{
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
		*argumentCount = i + 1;
		
		// Skip argument content
		while(*command != ' ')
			if(*command++ == '\0')
				return args;
	}
	return args;
}


/**** MEMJAM *****/
extern void attackercode(uint64_t addr);
static uint8_t attackerBuffer[0x2000];
static void attacker(void *args)
{
	int core = *((int *)args);
	set_thread_affinity(core);
	
	uint64_t addr = (((uint64_t)attackerBuffer & ~0xFFF) + 0x1110);
	printf_locked("Attacker targets %016x\n", addr);
	attackercode(addr);
}

extern uint64_t victimcode(uint64_t addr, int count);
static uint8_t victimBuffer[0x2000];
static void victim(void *args)
{
	int core = *((int *)args);
	set_thread_affinity(core);
	
	uint64_t addr = (((uint64_t)victimBuffer & ~0xFFF) + 0x1110);
	printf_locked("Victim targets %016x\n", addr);
	
	uint64_t sum = victimcode(addr, 100000) / 100000;
	printf_locked("MemJam time: %d\n", (int)sum);
}








/***** STFWD *******/


extern uint64_t measure(uint8_t *testBuffer);

#define ROUNDS 10000
#define PAGE_COUNT 2048
#define PAGE_SIZE 4096
#define WINDOW 32

void test_func(void *args)
{
	set_thread_affinity(*((int *)args));
	
	// TODO alloc contiguous
	
	printf_locked("malloc 1...\n");
	uint8_t *testBuffer = (uint8_t *)malloc(PAGE_COUNT * PAGE_SIZE);
	memset(testBuffer, 0, PAGE_COUNT * PAGE_SIZE);
	for(int p = 0; p < PAGE_COUNT / 8; ++p)
	{
		printf_locked("%4d    ", 8 * p);
		for(int i = 0; i < 8; ++i)
			printf_locked("%012x ", sys_virt_to_phy((uint64_t)&testBuffer[(8 * p + i) * PAGE_SIZE]));
		printf_locked("\n");
	}
	
	printf_locked("malloc 2...\n");
	uint16_t *measurementBuffer = (uint16_t *)malloc(PAGE_COUNT * sizeof(uint16_t));
	memset(measurementBuffer, 0, PAGE_COUNT * sizeof(uint16_t));
	
	printf_locked("measure...\n");
	for(int p = WINDOW; p < PAGE_COUNT; ++p)
	{
		uint64_t total = 0;
		
		for(int r = 0; r < ROUNDS; ++r)		
		{
			// Stores
			for(int i = p - WINDOW; i <= p; ++i)
				testBuffer[i * PAGE_SIZE] = 0;
			
			// Measuring load
			total += measure(testBuffer);
		}
		
		measurementBuffer[p] = (uint16_t)(total / ROUNDS);
	}
	
	terminal_draw(20, 20, 20, 20);

	for(int p = 0; p < PAGE_COUNT; ++p)
		if(measurementBuffer[p] > 250)
			printf_locked("%4d    %p -> %p    %u\n", p,  &testBuffer[p * PAGE_SIZE], sys_virt_to_phy((uint64_t)&testBuffer[p * PAGE_SIZE]), measurementBuffer[p]);

	printf_locked("free...\n");
	free(testBuffer);
	free(measurementBuffer);
}

void main()
{
	// Initialize library
	_start();
	
	// This thread should run on Core #0 by default, but do not rely on this
	set_thread_affinity(0);
	
	// Banner
	printf_locked("--- ITS Micro Kernel :: UI PROCESS ---\n");
	
	// Install handler for render context switch
	printf_locked("Installing context switch handlers...");
	for(int c = VKEY_F1; c <= VKEY_F12; ++c)
		register_keypress_handler(c, &handle_window_switch);
	printf_locked("OK\n");
	
	// Create file system
	printf_locked("Creating RAM file system...");
	if(create_directory("/", "in") != RAMFS_ERR_OK)
		printf_locked("failed\n");
	else
		printf_locked("OK\n");
	
	// Start LWIP thread
	printf_locked("Starting LWIP thread...\n");
	char *addressData[] = { ipAddress, subnetMask, gatewayAddress };
	run_thread(&itslwip_run, addressData, "lwip");
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
		else if(strcmp(args[0], "help") == 0)
		{
			// Print help text
			printf_locked("Supported commands:\n");
			printf_locked("    lss                       List remote directory\n");
			printf_locked("    ls                        List local file system tree\n");
			printf_locked("    dl [file name]            Download file from server to /in directory\n");
			printf_locked("    ul [file path]            Upload file to server\n");
			printf_locked("    prefix [file name] [n]    Show first n bytes of the given file\n");
			printf_locked("    start [file name]         Run the given ELF64 file as a new process\n");
			printf_locked("    sysinfo                   Print system information (e.g. CPU topology)\n");
			printf_locked("    reboot                    Reset the CPU\n");
		}
		else if(strcmp(args[0], "lss") == 0)
		{
			// Connect to server and send command
			tcp_handle_t tcpHandle = itslwip_connect(serverIpAddress, 17571);
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
			// Show file system tree
			dump_files();
		}
		else if(strcmp(args[0], "dl") == 0)
		{
			// Get file name
			if(argCount < 2)
				printf_locked("Missing argument.\n");
			else
			{
				int filenameLength = strlen(args[1]);
				char *filename = (char *)malloc(filenameLength + 1);
				strncpy(filename, args[1], filenameLength);
				filename[filenameLength] = '\n';
				
				// Connect to server and send command
				tcp_handle_t tcpHandle = itslwip_connect(serverIpAddress, 17571);
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
				fs_err_t err = create_file("/in", args[1], fileData, fileSize);
				if(err != RAMFS_ERR_OK)
					printf_locked("Error saving received file: %d.\n", err);
				
				// Disconnect
				itslwip_send_string(tcpHandle, "exit\n", 5);
				itslwip_disconnect(tcpHandle);
				free(fileData);
				free(filename);
			}
		}
		else if(strcmp(args[0], "ul") == 0)
		{
			// Check parameters
			if(argCount < 2)
				printf_locked("Missing argument.\n");
			else
			{
				// Check whether file exists
				int fileSize;
				uint8_t *fileData;
				fs_err_t err = get_file(args[1], (void **)&fileData, &fileSize);
				if(err != RAMFS_ERR_OK)
					printf_locked("Could not read file contents: %d\n", err);
				else
				{
					// Split path to get file name
					int filenameStartIndex;
					int pathLength = strlen(args[1]);
					for(filenameStartIndex = pathLength - 1; filenameStartIndex > 0; --filenameStartIndex)
						if(args[1][filenameStartIndex - 1] == '/')
							break;
					int filenameLength = pathLength - filenameStartIndex;
					printf("File name is '%s' (length %d)\n", &args[1][filenameStartIndex], filenameLength);
					char filenameBuffer[65]; // Maximum file name length and new line character
					strncpy(filenameBuffer, &args[1][filenameStartIndex], filenameLength);
					filenameBuffer[filenameLength] = '\n';
					
					// Connect to server and send command including file name
					tcp_handle_t tcpHandle = itslwip_connect(serverIpAddress, 17571);
					itslwip_send_string(tcpHandle, "sendout\n", 8);
					itslwip_send_string(tcpHandle, filenameBuffer, filenameLength + 1);
					
					// Send file size
					char fileSizeBuffer[11];
					itoa(fileSize, fileSizeBuffer, 10);
					int fileSizeLength = strlen(fileSizeBuffer);
					fileSizeBuffer[fileSizeLength] = '\n';
					itslwip_send_string(tcpHandle, fileSizeBuffer, fileSizeLength + 1);
					
					// Send file data
					itslwip_send(tcpHandle, fileData, fileSize);
					
					// Disconnect
					itslwip_send_string(tcpHandle, "exit\n", 5);
					itslwip_disconnect(tcpHandle);
					free(fileData);
				}
			}
		}
		else if(strcmp(args[0], "prefix") == 0)
		{
			// Load file
			if(argCount < 3)
				printf_locked("Missing argument.\n");
			else
			{
				uint8_t *fileData;
				int fileLength;
				if(get_file(args[1], (void **)&fileData, &fileLength) == RAMFS_ERR_OK)
				{
					// Print first byte
					int dumpLength = atoi(args[2]);
					if(dumpLength > fileLength)
						dumpLength = fileLength;
					for(int i = 0; i < dumpLength; ++i)
						printf_locked("%c", fileData[i]);
					free(fileData);
				}
				else
					printf_locked("File not found.\n");
			}
		}
		else if(strcmp(args[0], "start") == 0)
		{
			// Load program file
			if(argCount < 2)
				printf_locked("Missing argument.\n");
			else
			{
				uint8_t *elfData;
				int elfLength;
				if(get_file(args[1], (void **)&elfData, &elfLength) == RAMFS_ERR_OK)
				{
					// Try to start process
					printf_locked("Starting process...");
					if(start_process(elfData, elfLength))
						printf_locked("OK\n");
					else
						printf_locked("failed\n");
				}
				else
					printf_locked("Program file not found.\n");
			}
		}
		else if(strcmp(args[0], "arp") == 0)
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
				0x8D, 0x53, 0x3E, 0x2C,
				0xFF, 0xFF, 0xFF, 0xFF,	0xFF, 0xFF,
				0x8D, 0x53, 0x3E, 0xE8
			};
			sys_send_network_packet(packet, sizeof(packet));
			printf("Packet sent.\n");
		}
		else if(strcmp(args[0], "sysinfo") == 0)
		{
			// Retrieve processor count
			uint32_t processorCount;
			sys_info(0, (uint8_t *)&processorCount);
			printf("CPU count: %d\n", processorCount);
			
			// Retrieve topology data
			uint32_t *topologyBuffer = (uint32_t *)malloc(processorCount * 12);
			sys_info(1, (uint8_t *)topologyBuffer);
			printf("CPU topology data:\n");
			for(uint32_t p = 0; p < processorCount; ++p)
			{
				// Print data of this processor
				printf("    CPU #%d:\n", p);
				printf("        Package: %d\n", topologyBuffer[3 * p + 0]);
				printf("        Core: %d\n", topologyBuffer[3 * p + 1]);
				printf("        Thread: %d\n", topologyBuffer[3 * p + 2]);
			}
			
			// Done
			free(topologyBuffer);
		}
		else if(strcmp(args[0], "reboot") == 0)
		{
			// Reboot
			printf_locked("Initiating reboot...\n");
			sys_reset();
		}
		else if(strcmp(args[0], "test") == 0)
		{
			if(argCount < 2)
				printf_locked("Missing argument.\n");
			else
			{
				int core = atoi(args[1]);
				printf_locked("Starting measurement on core %d...\n", core);
				run_thread(test_func, &core, "measure");
			}
		}
		else if(strcmp(args[0], "memjam") == 0)
		{
			if(argCount < 4)
				printf_locked("Missing argument(s). Usage [attacker core] [victim#1 core] [victim#2 core]\n");
			else
			{
				int coreAttacker = atoi(args[1]);
				int coreVictim1 = atoi(args[2]);
				int coreVictim2 = atoi(args[3]);
				printf_locked("Starting attacker on core %d...\n", coreAttacker);
				run_thread(attacker, &coreAttacker, "memjamattack");
				
				for(int i = 0; i < 1000000000; ++i)
					__asm ("nop");
				
				printf_locked("Starting victim on core %d...\n", coreVictim1);
				run_thread(victim, &coreVictim1, "memjamvictim1");
				
				for(int i = 0; i < 1000000000; ++i)
					__asm ("nop");
				
				printf_locked("Starting victim on core %d...\n", coreVictim2);
				run_thread(victim, &coreVictim2, "memjamvictim2");
			}
		}
		else if(strcmp(args[0], "scramble") == 0)
		{
			printf_locked("Allocate...\n");
			sys_test();
			
			
			printf_locked("Done\n");
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