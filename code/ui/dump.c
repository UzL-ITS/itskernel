/*
ITS kernel dump functionality.
*/

/* INCLUDES */

#include "dump.h"
#include <internal/syscall/syscalls.h>


/* FUNCTIONS */

void create_dump(dump_type_t type, const char *filePath)
{
	// Generate dump
	sys_dump((int)type, filePath);
}