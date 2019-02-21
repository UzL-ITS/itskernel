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
#include <itslwip.h>
#include <internal/terminal/terminal.h>
#include "dump.h"


/* VARIABLES */

// Colors.
const color_t COLOR_CURRENT_DIRECTORY = { 0, 200, 0 };
const color_t COLOR_ERROR = { 200, 0, 0 };

// Network configuration.
/*
static char serverIpAddress[] = "141.83.62.232";
static char ipAddress[] = "141.83.62.44";
static char subnetMask[] = "255.255.255.0";
static char gatewayAddress[] = "141.83.62.1";
/*/
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
		
		// Skip argument content and potential new line characters
		while(*command != ' ')
			if(*command++ == '\0')
				return args;
			else if(*command == '\n')
				*command++ = '\0';
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
	if(create_directory("/", "in") != FS_ERR_OK)
		printf_locked("failed\n");
	else
		printf_locked("OK\n");
	
	// Set current directory
	char currentDirectory[1024] = "/";
	
	// Start LWIP thread
	printf_locked("Starting LWIP thread...\n");
	char *addressData[] = { ipAddress, subnetMask, gatewayAddress };
	run_thread(&itslwip_run, addressData, "lwip");
	printf_locked("Thread started.\n");
	
	// Command loop
	char buffer[16384]; // String buffer for arbitrary use
	while(true)
	{
		// Print prompt
		terminal_set_front_color(COLOR_CURRENT_DIRECTORY);
		printf_locked("\n%s", currentDirectory);
		terminal_reset_colors();
		printf_locked("$ ");
		
		// Read command
		char *command = getline();
		int argCount;
		char **args = split_command_string(command, &argCount);
		//for(int i = 0; i < argCount; ++i)
		//	printf_locked("arg %d: >>%s<<\n", i, args[i]);
		if(argCount < 1)
		{
			terminal_set_front_color(COLOR_ERROR);
			printf_locked("Please type a command, or \"help\" for usage information.\n");
			continue;
		}
		
		// Handle command
		if(strcmp(args[0], "exit") == 0)
			break;
		else if(strcmp(args[0], "help") == 0)
		{
			// Print help text
			printf_locked
			(
				"Supported commands:\n"
				"    lss <protocol>                List remote directory\n"
				"    ls                            List current directory\n"
				"    cd <directory name>           Change to given directory\n"
				"    mkdir <directory name>        Create directory\n"
				"    dl <protocol> <file name>     Download file from server to /in directory\n"
				"    ul <protocol> <file path>     Upload file to server\n"
				"    cat <file path>               Print contents of given file\n"
				"    start <file path>             Run the given ELF64 file as a new process\n"
				"    sysinfo                       Print system information (e.g. CPU topology)\n"
				"    reboot                        Reset the CPU\n"
				"\n"
				"Supported protocols: tcp udp\n"
				"\n"
				"Note: File and directory names can contain arbitrary characters, except spaces and slashes.\n"
			);
		}
		else if(strcmp(args[0], "lss") == 0)
		{
			if(argCount < 2)
			{
				terminal_set_front_color(COLOR_ERROR);
				printf_locked("Missing argument.\n");
			}
			else
			{
				// Connect to server and send command
				conn_handle_t connHandle = itslwip_connect(serverIpAddress, 17571, strcmp(args[1], "tcp") == 0);
				itslwip_send_string(connHandle, "ls\n", 3);
				
				// Receive file count
				itslwip_receive_line(connHandle, buffer, sizeof(buffer));
				int count = atoi(buffer);
				printf_locked("Server has %d input files:\n", count);
				
				// Receive file list
				for(int i = 0; i < count; ++i)
				{
					itslwip_receive_line(connHandle, buffer, sizeof(buffer));
					printf_locked("    %s\n", buffer);
				}
				
				// Disconnect
				itslwip_send_string(connHandle, "exit\n", 5);
				itslwip_disconnect(connHandle);
			}
		}
		else if(strcmp(args[0], "ls") == 0 || strcmp(args[0], "ll") == 0)
		{
			// Retrieve directory contents
			int lsLength = flist(currentDirectory, buffer, sizeof(buffer) - 1);
			buffer[lsLength] = '\0';
			printf_locked("%s", buffer);
		}
		else if(strcmp(args[0], "cd") == 0)
		{
			// Get directory name
			if(argCount < 2)
			{
				terminal_set_front_color(COLOR_ERROR);
				printf_locked("Missing argument.\n");
			}
			else
			{
				// Move one directory up?
				int currentDirectoryStringLength = strlen(currentDirectory);
				int nextDirectoryNameLength = strlen(args[1]);
				if(nextDirectoryNameLength >= 2 && args[1][0] == '.' && args[1][1] == '.' && currentDirectoryStringLength > 1)
				{
					// Go back to previous slash
					int prevSlashPos;
					for(prevSlashPos = currentDirectoryStringLength - 2; prevSlashPos > 0; --prevSlashPos)
						if(currentDirectory[prevSlashPos] == '/')
							break;
					
					// Add terminating zero after slash
					currentDirectory[prevSlashPos + 1] = '\0';
				}
				else
				{
					// Remove slashes from back of directory name, if there are any
					while(args[1][nextDirectoryNameLength - 1] == '/')
						args[1][--nextDirectoryNameLength] = '\0';
					
					// Check whether directory exists
					if(test_directory(currentDirectory, args[1]) != FS_ERR_DIRECTORY_EXISTS)
					{
						terminal_set_front_color(COLOR_ERROR);
						printf_locked("Can not find specified directory.\n");
					}
					else
					{
						// Change into directory
						if(currentDirectoryStringLength + nextDirectoryNameLength >= (int)sizeof(currentDirectory) - 2)
						{
							terminal_set_front_color(COLOR_ERROR);
							printf_locked("Could not change into directory: Path length exceeds buffer.\n");
						}
						else
						{
							// Add name
							strncpy(&currentDirectory[currentDirectoryStringLength], args[1], nextDirectoryNameLength);
							currentDirectoryStringLength += nextDirectoryNameLength;
							
							// Append slash and terminating 0-byte
							currentDirectory[currentDirectoryStringLength++] = '/';
							currentDirectory[currentDirectoryStringLength++] = '\0';
						}
					}
				}
			}
		}
		else if(strcmp(args[0], "mkdir") == 0)
		{
			// Get directory name
			if(argCount < 2)
			{
				terminal_set_front_color(COLOR_ERROR);
				printf_locked("Missing argument.\n");
			}
			else
			{
				// Try to create directory
				fs_err_t err = create_directory(currentDirectory, args[1]);
				if(err != FS_ERR_OK)
				{
					terminal_set_front_color(COLOR_ERROR);
					printf_locked("Can not create directory: %d.\n", err);
				}
			}
		}
		else if(strcmp(args[0], "dl") == 0)
		{
			// Get file name
			if(argCount < 3)
			{
				terminal_set_front_color(COLOR_ERROR);
				printf_locked("Missing argument.\n");
			}
			else
			{
				int filenameLength = strlen(args[2]);
				char *filename = (char *)malloc(4 + filenameLength + 1);
				strncpy(filename, "/in/", 4);
				strncpy(&filename[4], args[2], filenameLength);
				filename[4 + filenameLength] = '\n';
				
				// Connect to server and send command
				conn_handle_t connHandle = itslwip_connect(serverIpAddress, 17571, strcmp(args[1], "tcp") == 0);
				itslwip_send_string(connHandle, "sendin\n", 7);
				itslwip_send_string(connHandle, &filename[4], filenameLength + 1);
				
				// Receive file size
				itslwip_receive_line(connHandle, buffer, sizeof(buffer));
				int fileSize = atoi(buffer);
				printf_locked("File size: %d bytes\n", fileSize);
				
				// Receive file data
				uint8_t *fileData = (uint8_t *)malloc(fileSize);
				itslwip_receive_data(connHandle, fileData, fileSize);
				
				// Store file
				fs_fd_t fd;
				filename[4 + filenameLength] = '\0';
				printf_locked("Storing downloaded data in %s...\n", filename);
				fs_err_t err = fopen(filename, &fd, true);
				if(err != FS_ERR_OK)	
				{
					terminal_set_front_color(COLOR_ERROR);
					printf_locked("Error opening handle for writing received file: %d.\n", err);
				}
				else
				{
					// Write data
					if((int)fwrite(fileData, fileSize, fd) != fileSize)
					{
						terminal_set_front_color(COLOR_ERROR);
						printf_locked("Error storing downloaded data.\n");
					}
					
					// Done
					fclose(fd);
				}
				
				// Disconnect
				itslwip_send_string(connHandle, "exit\n", 5);
				itslwip_disconnect(connHandle);
				free(fileData);
				free(filename);
			}
		}
		else if(strcmp(args[0], "ul") == 0)
		{
			// Check parameters
			if(argCount < 3)
			{
				terminal_set_front_color(COLOR_ERROR);
				printf_locked("Missing argument.\n");
			}
			else
			{
				// Absolute or relative path?
				int pathLength = strlen(args[2]);
				if(pathLength >= 1 && args[2][0] == '/')
				{
					// Use argument as entire path
					strncpy(buffer, args[2], pathLength);
					buffer[pathLength] = '\0';
				}
				else
				{
					// Concat current directory and given path
					int currentDirectoryStringLength = strlen(currentDirectory);
					strncpy(buffer, currentDirectory, currentDirectoryStringLength);
					strncpy(&buffer[currentDirectoryStringLength], args[2], pathLength);
					buffer[currentDirectoryStringLength + pathLength] = '\0';
				}
				
				// Open file
				fs_fd_t fd;
				fs_err_t err = fopen(buffer, &fd, false);
				if(err != FS_ERR_OK)
				{
					terminal_set_front_color(COLOR_ERROR);
					printf_locked("Could not open file: %d\n", err);
				}
				else
				{
					// Retrieve file size
					fseek(0, FS_SEEK_END, fd);
					uint64_t fileSize = ftell(fd);
					
					// Retrieve file contents
					uint8_t *fileData = malloc(fileSize);
					fseek(0, FS_SEEK_START, fd);
					fread(fileData, fileSize, fd);
					fclose(fd);
					
					// Split path to get file name
					int filenameStartIndex;
					int pathLength = strlen(buffer);
					for(filenameStartIndex = pathLength - 1; filenameStartIndex > 0; --filenameStartIndex)
						if(buffer[filenameStartIndex - 1] == '/')
							break;
					int filenameLength = pathLength - filenameStartIndex;
					printf("File name is '%s' (length %d)\n", &buffer[filenameStartIndex], filenameLength);
					char filenameBuffer[65]; // Maximum file name length and new line character
					strncpy(filenameBuffer, &buffer[filenameStartIndex], filenameLength);
					filenameBuffer[filenameLength] = '\n';
					
					// Connect to server and send command including file name
					conn_handle_t connHandle = itslwip_connect(serverIpAddress, 17571, strcmp(args[1], "udp") == 0);
					itslwip_send_string(connHandle, "sendout\n", 8);
					itslwip_send_string(connHandle, filenameBuffer, filenameLength + 1);
					
					// Send file size
					char fileSizeBuffer[11];
					itoa((int)fileSize, fileSizeBuffer, 10);
					int fileSizeLength = strlen(fileSizeBuffer);
					fileSizeBuffer[fileSizeLength] = '\n';
					itslwip_send_string(connHandle, fileSizeBuffer, fileSizeLength + 1);
					
					// Send file data
					itslwip_send(connHandle, fileData, fileSize);
					
					// Disconnect
					itslwip_send_string(connHandle, "exit\n", 5);
					itslwip_disconnect(connHandle);
					free(fileData);
				}
			}
		}
		else if(strcmp(args[0], "cat") == 0)
		{
			// Check arguments
			if(argCount < 2)
			{
				terminal_set_front_color(COLOR_ERROR);
				printf_locked("Missing argument.\n");
			}
			else
			{
				// Absolute or relative path?
				int pathLength = strlen(args[1]);
				if(pathLength >= 1 && args[1][0] == '/')
				{
					// Use argument as entire path
					strncpy(buffer, args[1], pathLength);
					buffer[pathLength] = '\0';
				}
				else
				{
					// Concat current directory and given path
					int currentDirectoryStringLength = strlen(currentDirectory);
					strncpy(buffer, currentDirectory, currentDirectoryStringLength);
					strncpy(&buffer[currentDirectoryStringLength], args[1], pathLength);
					buffer[currentDirectoryStringLength + pathLength] = '\0';
				}
				
				// Open file
				fs_fd_t fd;
				fs_err_t err = fopen(buffer, &fd, false);
				if(err != FS_ERR_OK)
				{
					terminal_set_front_color(COLOR_ERROR);
					printf_locked("Could not open file: %d\n", err);
				}
				else
				{
					// Read file until end is reached
					int bytesRead;
					while((bytesRead = (int)fread((uint8_t *)buffer, sizeof(buffer) - 1, fd)) > 0)
					{
						// Mask non-printable characters
						for(char *bufferPtr = &buffer[bytesRead - 1]; bufferPtr >= &buffer[0]; --bufferPtr)
						{
							char c = *bufferPtr;
							if(!((c >= 0x20 && c < 0x7F) || c == '\n' || c == '\r'))
								*bufferPtr = '?';
						}
						
						// Print contents
						buffer[bytesRead] = '\0';
						printf_locked("%s", buffer);
					}
					printf_locked("\n");
					
					// Done
					fclose(fd);
				}
			}
		}
		else if(strcmp(args[0], "start") == 0)
		{
			// Load program file
			if(argCount < 2)
			{
				terminal_set_front_color(COLOR_ERROR);
				printf_locked("Missing argument.\n");
			}
			else
			{
				// Absolute or relative path?
				int pathLength = strlen(args[1]);
				if(pathLength >= 1 && args[1][0] == '/')
				{
					// Use argument as entire path
					strncpy(buffer, args[1], pathLength);
					buffer[pathLength] = '\0';
				}
				else
				{
					// Concat current directory and given path
					int currentDirectoryStringLength = strlen(currentDirectory);
					strncpy(buffer, currentDirectory, currentDirectoryStringLength);
					strncpy(&buffer[currentDirectoryStringLength], args[1], pathLength);
					buffer[currentDirectoryStringLength + pathLength] = '\0';
				}
				
				// Check whether program file exists
				fs_fd_t fd;
				fs_err_t err = fopen(buffer, &fd, false);
				if(err != FS_ERR_OK)
				{
					terminal_set_front_color(COLOR_ERROR);
					printf_locked("Could not open executable file: %d\n", err);
				}
				else
				{
					// Check successful
					fclose(fd);
				
					// Start process
					printf_locked("Starting process...");
					if(start_process(buffer))
						printf_locked("OK\n");
					else
						printf_locked("failed\n");
				}
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
		else if(strcmp(args[0], "memory") == 0)
		{
			if(argCount < 2)
			{
				// Print help text for sub commands
				printf_locked("Supported commands:\n");
				printf_locked("    dump [file name]      Generate list of available physical pages and store it in the given file\n");
				printf_locked("    showmap [file name]   Render map of physical memory\n");
			}
			else if(strcmp(args[1], "dump") == 0)
			{
				if(argCount < 3)
				{
					terminal_set_front_color(COLOR_ERROR);
					printf_locked("Missing argument.\n");
				}
				else
				{
					// Absolute or relative path?
					int pathLength = strlen(args[2]);
					if(pathLength >= 1 && args[2][0] == '/')
					{
						// Use argument as entire path
						strncpy(buffer, args[2], pathLength);
						buffer[pathLength] = '\0';
					}
					else
					{
						// Concat current directory and given path
						int currentDirectoryStringLength = strlen(currentDirectory);
						strncpy(buffer, currentDirectory, currentDirectoryStringLength);
						strncpy(&buffer[currentDirectoryStringLength], args[2], pathLength);
						buffer[currentDirectoryStringLength + pathLength] = '\0';
					}
					
					// Generate dump
					printf_locked("Running dump...");
					create_dump(DUMP_PMM_STATE, buffer);
					printf_locked("done.\n");
				}
			}
			else if(strcmp(args[1], "showmap") == 0)
			{
				
			}
		}
		else if(strcmp(args[0], "reboot") == 0)
		{
			// Reboot
			printf_locked("Initiating reboot...\n");
			sys_reset();
		}
		else if(strcmp(args[0], "memjam") == 0)
		{
			if(argCount < 4)
			{
				terminal_set_front_color(COLOR_ERROR);
				printf_locked("Missing argument(s). Usage [attacker core] [victim#1 core] [victim#2 core]\n");
			}
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
			if(argCount < 2)
			{
				terminal_set_front_color(COLOR_ERROR);
				printf_locked("Missing argument.\n");
			}
			else
			{
				int mode = atoi(args[1]);
				sys_custom(mode);
				
				printf_locked("Done\n");
			}
		}
		else
		{
			terminal_set_front_color(COLOR_ERROR);
			printf_locked("Unknown command.\n");
		}
		
		// Free command string
		free(args);
		free(command);
	}
		
	// Exit with return code
	printf_locked("Exiting...\n");
	_end(0);
}