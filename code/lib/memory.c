/*
ITS kernel user space memory manager.
*/

/* INCLUDES */
#include <memory.h>
#include <internal/syscall/syscalls.h>


/* VARIABLES */




/* FUNCTIONS */

void *heap_alloc(int size)
{
	// Parameter checking is done by the kernel
	return sys_heap_alloc(size);
}

void heap_free(void *memory)
{
	// Call kernel function
	sys_heap_free(memory);
}