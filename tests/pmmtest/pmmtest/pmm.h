
#ifndef _MM_PMM_H
#define _MM_PMM_H

#include <stdint.h>

/* where the stacks start in virtual memory */
#define VM_STACK_OFFSET 0xFFFFFEFEFFFF7000

/* the ids of the memory zones */
#define ZONE_DMA   0
#define ZONE_DMA32 1
#define ZONE_STD   2
#define ZONE_COUNT 3

/* the limits of the DMA and DMA32 memory zones */
#define ZONE_LIMIT_DMA   0xFFFFFF   /* 2^24 - 1 */
#define ZONE_LIMIT_DMA32 0xFFFFFFFF /* 2^32 - 1 */

void pmm_init(uint64_t addr_start, uint64_t addr_end, uint64_t baseOffset, uintptr_t phyStack);
uintptr_t pmm_alloc(void);
uintptr_t pmm_allocs(int size);
uintptr_t pmm_allocz(int zone);
uintptr_t pmm_allocsz(int size, int zone);

// Frees the given 4K page. Make sure it *is* indeed a 4K page, else the PMM might break.
void pmm_free(uintptr_t addr);

// Frees the given page with the given size. Make sure you are using the correct page size, else the PMM might break.
void pmm_frees(int size, uintptr_t addr);

// Tries to allocate "count" contiguous pages of the given size.
// If the allocation fails, 0 is returned.
uintptr_t pmm_alloc_contiguous(int size, int count);

// Dumps the internal PMM stack to a file. Only for debugging.
void pmm_dump_stack(const char *name);

#endif
