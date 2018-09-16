/*
ITS kernel standard library threading functionality.
*/

/* INCLUDES */

#include <threading/process.h>
#include <internal/syscall/syscalls.h>


/* VARIABLES */


/* FUNCTIONS */

bool start_process(uint8_t *program, int programLength)
{
	// Sanity check
	if(!program || programLength <= 0)
		return false;
	
	// Do system call
	return sys_start_process(program, programLength);
}