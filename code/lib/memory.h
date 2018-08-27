#pragma once

/*
ITS kernel user space memory manager.

Currently supported:
	- Allocation of n*4KB sized blocks on the heap

TODO add real malloc implementation like dlmalloc or ptmalloc3
*/

/* INCLUDES */


/* DECLARATIONS */

// Allocates memory on the heap. The size is always a multiple of 4096 Bytes (4 KB).
void *heap_alloc(int size);

// Frees an allocated heap memory block.
void heap_free(void *memory);