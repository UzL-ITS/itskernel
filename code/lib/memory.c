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

void *memcpy(void *destination, const void *source, int length)
{
	// Copy byte wise
	uint8_t *destinationBytes = (uint8_t *)destination;
	const uint8_t *sourceBytes = (uint8_t *)source;
	while(length--)
		*destinationBytes++ = *sourceBytes++;
	
	return destination;
}

void *memset(void *array, int value, int length)
{
	// Memset byte wise
	uint8_t *ptr = (uint8_t *)array;
	while(length--)
		*ptr++ = (uint8_t)value;
	
	return array;
}

int memcmp(const void *array1, const void *array2, int length)
{
	// Compare byte wise
    const uint8_t *ptr1 = array1;
	const uint8_t *ptr2 = array2;
    while(length--)
        if(*ptr1 != *ptr2)
            return *ptr1 - *ptr2;
        else
		{
            ++ptr1;
			++ptr2;
		}
    return 0;
}