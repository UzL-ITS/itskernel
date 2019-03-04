
#ifndef _MM_PMM_H
#define _MM_PMM_H

#include <mm/common.h>
#include <util/list.h>
#include <stdint.h>

// The different memory zones.
#define ZONE_DMA   0
#define ZONE_DMA32 1
#define ZONE_STD   2
#define ZONE_COUNT 3

// Memory zone limits.
#define ZONE_LIMIT_DMA   0xFFFFFF   /* 2^24 - 1 */
#define ZONE_LIMIT_DMA32 0xFFFFFFFF /* 2^32 - 1 */

// The amount of size/zone stacks.
#define STACK_COUNT (ZONE_COUNT * SIZE_COUNT)

// Auxiliary pages for bookkeeping in defragmentation algorithm; this value is safe for ~24 GiB physical RAM
#define PMM_AUX_PAGES_COUNT 64

// Virtual base address of the PMM management data.
// - 9 entries: Top stack pages
// - 1 entry: List of reserved pages to be used as stack pages later.
// - 64 entries: Auxiliary pages for bookkeeping in defragmentation.
#define PMM_INTERNAL_DATA_ADDRESS 0xFFFFFEFEFFE00000

void pmm_init(list_t *map);
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
// NOTE: This VERY SLOW function has a LARGE performance impact!
//       It is meant only for the few cases where contiguous physical memory is absolutely necessary (e.g. DMA).
uintptr_t pmm_alloc_contiguous(int size, int count);

// Dumps the internal PMM stack to a file.
// NOTE: This function does some estimations on the current amount of available page frames.
//       Doing allocations on another core while the dump is being generated might cause crashes.
void pmm_dump_stack(const char *name);

// Returns the amount of available physical memory.
uint64_t pmm_get_available_memory();

#endif
