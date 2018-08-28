/*
ITS kernel user space memory manager.
*/

/* INCLUDES */

#include <memory.h>
#include <stdint.h>
#include <internal/syscall/syscalls.h>


/* VARIABLES */




/* FUNCTIONS */

void *malloc(int size)
{
	// TODO add real implementation
	return heap_alloc(size);
}

void free(void *memory)
{
	// TODO add real implementation
	heap_free(memory);
}

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

void memcpy(void *destination, const void *source, int length)
{
	// Copy byte wise
	uint8_t *destinationBytes = (uint8_t *)destination;
	const uint8_t *sourceBytes = (uint8_t *)source;
	while(length--)
		*destinationBytes++ = *sourceBytes++;
}
