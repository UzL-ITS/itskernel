#pragma once

/*
ITS kernel user space memory manager.

Currently supported:
	- Allocation of n*4KB sized blocks on the heap

TODO add real malloc implementation like dlmalloc or ptmalloc3
*/

/* INCLUDES */


/* DECLARATIONS */

// Allocates memory of arbitrary size on the heap.
void *malloc(int size);

// Frees memory allocated by malloc().
void free(void *memory);

// Allocates memory on the heap. The size is always a multiple of 4096 Bytes (4 KB).
void *heap_alloc(int size);

// Frees memory allocated by heap_alloc().
void heap_free(void *memory);

// Copies memory from source to destination. The arrays must not intersect!
void *memcpy(void *destination, const void *source, int length);

// Sets the first _length_ bytes in the target array to a given value.
void *memset(void *array, int value, int length);

// Compares the given two byte arrays and returns 0 on equality.
int memcmp(const void *array1, const void *array2, int length);

// Copies memory from source to destination. Unlike memcpy(), the arrays are allowed to intersect.
void *memmove(void *destination, const void *source, int length);