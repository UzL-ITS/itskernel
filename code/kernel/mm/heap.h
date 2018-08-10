
#ifndef _MM_HEAP_H
#define _MM_HEAP_H

#include <mm/common.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Initializes the kernel heap by allocating an initial free block which covers
 * all of the free virtual address space from the end of the kernel image up to
 * the miscallenous reserved space at the end (which is used for physical
 * memory manager stacks, mapping the 4GB physical address space into virtual
 * memory and the recursive page directory trick.)
 */
void heap_init(void);

/*
 * Reserves 'size' bytes of memory, rounded up to the nearest page size, on the
 * kernel heap. The virtual memory is not automatically mapped to any physical
 * memory.
 */
void *heap_reserve(size_t size);

/*
 * Reserves 'size' bytes of memory on the kernel heap, like the function above,
 * and then allocates physical frames for this memory, and maps the physical
 * frames into the virtual memory.
 */
void *heap_alloc(size_t size, vm_acc_t flags);

// Same as heap_alloc, but makes sure that the allocated physical pages are contiguous.
// The physical address of the allocated memory is written to the variable pointed to by physicalAddressPtr.
void *heap_alloc_contiguous(size_t size, vm_acc_t flags, uint64_t *physicalAddressPtr);

/*
 * Frees some memory from the kernel heap. If the memory was allocated with
 * heap_alloc(), the physical frames will automatically be freed and unmapped
 * from virtual memory.
 */
void heap_free(void *ptr);

/* Prints out the kernel heap blocks for debugging purposes. */
void heap_trace(void);

#endif
