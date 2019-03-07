/*
Instruction fuzzing.
*/

/* INCLUDES */

#include <app.h>
#include <io.h>
#include <memory.h>
#include <stdbool.h>
#include <stdint.h>
#include <internal/syscall/syscalls.h>


/* FUNCTIONS */

void main()
{
	// Initialize library
	_start();
	
	printf_locked("Hello world!\n");
	
	// Exit with return code
	_end(0);
}