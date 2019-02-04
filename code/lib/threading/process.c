/*
ITS kernel standard library threading functionality.
*/

/* INCLUDES */

#include <threading/process.h>
#include <internal/syscall/syscalls.h>


/* VARIABLES */


/* FUNCTIONS */

bool start_process(const char *programPath)
{
	// Sanity check
	if(!programPath)
		return false;
	
	// Do system call
	return sys_start_process(programPath);
}