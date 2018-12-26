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
	void *data;
	int dataLength;
	printf("Read file: %d\n", get_file("/in/testapp", &data, &dataLength));
	printf("File length is %d\n", dataLength);
	printf("Write file: %d\n", create_file("/in", "testapp2", data, dataLength));
	
	// Exit with return code
	_end(0);
}