/*
Kernel UI process main file.
*/

/* INCLUDES */

#include <app.h>
#include <io.h>
#include <threading/thread.h>
#include <threading/process.h>
#include <memory.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>


/* FUNCTIONS */

void main()
{
	// Initialize library
	_start();
	
	// Banner
	printf_locked("Hello world from the first test application!\n");
	
	// Read own program file and save a copy of it
	fs_fd_t fdIn;
	fs_fd_t fdOut;
	printf("fopen 1: %d\n", fopen("/in/test.elf", &fdIn, false));
	printf("fopen 2: %d\n", fopen("/in/test2.elf", &fdOut, true));
	fseek(0, FS_SEEK_END, fdIn);
	int inSize = (int)ftell(fdIn);
	printf("Size: %d bytes\n", inSize);
	fseek(0, FS_SEEK_START, fdIn);
	uint8_t *buffer = malloc(inSize);
	printf("fread: %d\n", (int)fread(buffer, inSize, fdIn));
	printf("fwrite: %d\n", (int)fwrite(buffer, inSize, fdOut));
	fclose(fdIn);
	fclose(fdOut);
	
	printf("Done.\n");
	
	// Exit with return code
	_end(0);
}