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
	
	// Exit with return code
	_end(0);
}