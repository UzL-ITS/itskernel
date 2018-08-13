
#include <mm/pmm.h>
#include <mm/align.h>
#include <mm/common.h>
#include <mm/map.h>
#include <cpu/tlb.h>
#include <lock/spinlock.h>
#include <util/container.h>
#include <trace/trace.h>
#include <stdlib/string.h>

#define PAGE_TABLE_OFFSET 0xFFFFFF7F7F7FF000
#define STACK_COUNT (ZONE_COUNT * SIZE_COUNT)
#define PMM_STACK_SIZE (PAGE_TABLE_ENTRY_COUNT - 2)
#define SZ_TO_IDX(s,z) ((s) * ZONE_COUNT + (z))

typedef struct
{
	uint64_t next;
	uint64_t count;
	uint64_t frames[PMM_STACK_SIZE];
} __attribute__((__packed__)) pmm_stack_t;

// Pre-allocated memory for the nine stack pages.
// This memory is mapped in pmm_init() to the virtual addresses pointed to by VM_STACK_OFFSET.
static pmm_stack_t pmm_phy_stack[STACK_COUNT] __attribute__((__aligned__(FRAME_SIZE)));

// Pointer to the first of nine pages containing the PMM stacks for each zone.
static pmm_stack_t *pmm_stack = (pmm_stack_t *) VM_STACK_OFFSET;

// Points to the PML1 level table that contains the PMM stack pages (pmm_stack_pml1 in start.s).
// The given hardcoded address uses magic to do a loop in PML4 by accessing PML4[510], which contains a pointer to itself.
static uint64_t *pmm_page_table = (uint64_t *) PAGE_TABLE_OFFSET;

static spinlock_t pmm_lock = SPIN_UNLOCKED;

// Total page count for each size/zone combination. Only used for debugging.
static uint64_t pmm_counts[STACK_COUNT];

static const char *get_zone_str(int zone)
{
	switch (zone)
	{
		case ZONE_DMA:
			return "DMA	";

		case ZONE_DMA32:
			return "DMA32";

		case ZONE_STD:
			return "STD	";
	}

	return 0;
}

static const char *get_size_str(int size)
{
	switch (size)
	{
		case SIZE_4K:
			return "4K";

		case SIZE_2M:
			return "2M";

		case SIZE_1G:
			return "1G";
	}

	return 0;
}

// Returns the zone of the given physical address regarding a specific frame size.
static int get_zone(int size, uintptr_t addr)
{
	switch (size)
	{
		case SIZE_4K:
			addr += FRAME_SIZE - 1;
			break;

		case SIZE_2M:
			addr += FRAME_SIZE_2M - 1;
			break;

		case SIZE_1G:
			addr += FRAME_SIZE_1G - 1;
			break;
	}

	if(addr <= ZONE_LIMIT_DMA)
		return ZONE_DMA;
	else if(addr <= ZONE_LIMIT_DMA32)
		return ZONE_DMA32;
	else
		return ZONE_STD;
}

// Sets the PMM stack page "addr" as current for its given size/zone, and returns the previous address.
static uintptr_t stack_switch(int size, int zone, uintptr_t addr)
{
	int idx = SZ_TO_IDX(size, zone);
	int table_idx = PAGE_TABLE_ENTRY_COUNT - STACK_COUNT + idx;

	uintptr_t old_addr = pmm_page_table[table_idx] & PG_ADDR_MASK;
	pmm_page_table[table_idx] = addr | PG_PRESENT | PG_WRITABLE | PG_NO_EXEC;
	tlb_invlpg(VM_STACK_OFFSET + idx * FRAME_SIZE);

	return old_addr;
}

static void _pmm_free(int size, int zone, uintptr_t addr);

// Retrieve an address with the given size/zone from a matching stack.
static uintptr_t _pmm_alloc(int size, int zone)
{
	// Determine stack for the given size/zone combination
	int idx = SZ_TO_IDX(size, zone);
	pmm_stack_t *stackTop = &pmm_stack[idx];

	// Still addresses available on that stack page? -> Just return the top most one of them
	if(stackTop->count != 0)
		return stackTop->frames[--stackTop->count];
	
	// Stack page used up, is there another one?
	if(stackTop->next)
	{
		// Go to next stack page
		uintptr_t addr = stack_switch(size, zone, stackTop->next);
		
		// If the zone is OK, just recycle the address of the stack page we just discarded
		int stack_zone = get_zone(SIZE_4K, addr);
		if(size == SIZE_4K && stack_zone <= zone)
			return addr;
		else
		{
			// We cannot recycle this page, so free it and run the allocation function again (this time with a filled stack page)
			_pmm_free(SIZE_4K, stack_zone, addr);
			return _pmm_alloc(size, zone);
		}
	}
	
	// Unfortunately we need to split a 2M or 1G page
	if(size == SIZE_2M)
	{
		// Allocate a fresh 1G page (its address is removed from the respective stack)
		uintptr_t addr = _pmm_alloc(SIZE_1G, zone);
		if(addr)
		{
			// Mark last 511 2M pages of the 1G page as free
			for(uintptr_t off = FRAME_SIZE_2M; off < FRAME_SIZE_1G; off += FRAME_SIZE_2M)
				_pmm_free(SIZE_2M, zone, addr + off);

			// Return the first 2M page
			return addr;
		}
	}
	else if(size == SIZE_4K)
	{
		// Allocate a fresh 2M page (its address is removed from the respective stack)
		uintptr_t addr = _pmm_alloc(SIZE_2M, zone);
		if(addr)
		{
			// Mark last 511 4K pages of the 2M page as free
			for(uintptr_t off = FRAME_SIZE; off < FRAME_SIZE_2M; off += FRAME_SIZE)
				_pmm_free(SIZE_4K, zone, addr + off);

			// Return the first 4K page
			return addr;
		}
	}

	// Nothing worked, so finally try a smaller zone
	if(zone > ZONE_DMA)
		return _pmm_alloc(size, zone - 1);
	
	// Well, looks like RAM is completely used up (or terribly fragmented)
	return 0;
}

// Adds the given address back to its matching stack.
static void _pmm_free(int size, int zone, uintptr_t addr)
{
	// Get top most matching stack page
	int idx = SZ_TO_IDX(size, zone);
	pmm_stack_t *stackTop = &pmm_stack[idx];

	// Still space left on that stack page -> store addr there
	if(stackTop->count != PMM_STACK_SIZE)
	{
		stackTop->frames[stackTop->count++] = addr;
		return;
	}

	// If this is a 4K ZONE_STD page, just re-use it as the next PMM stack page
	if(size == SIZE_4K && zone == ZONE_STD)
	{
		// NOTE: After calling stack_switch() stackTop will point to the new entry, therefore setting stackTop->next is correct
		stackTop->next = stack_switch(size, zone, addr);
		stackTop->count = 0;
		return;
	}

	// Allocate new physical stack page
	uintptr_t new_addr = _pmm_alloc(SIZE_4K, ZONE_STD);
	if(new_addr)
	{
		// NOTE: After calling stack_switch() stackTop will point to the new entry, therefore setting stackTop->next is correct
		stackTop->next = stack_switch(size, zone, new_addr);
		stackTop->count = 0;
		
		// Add the freed address to the current stack page
		stackTop->frames[stackTop->count++] = addr;
		return;
	}
	
	// Allocation in ZONE_STD failed (probably physical RAM too small)
	// TODO page splitting is not really optimal, it might randomly divide a 1G page into many pieces, making contiguous allocation difficult (see also alloc function, this may be optimized as well)
	if(size == SIZE_4K)
	{
		// This is a 4K page in an arbitrary zone, just use it
		// NOTE: After calling stack_switch() stackTop will point to the new entry, therefore setting stackTop->next is correct
		stackTop->next = stack_switch(size, zone, addr);
		stackTop->count = 0;
	}
	else if(size == SIZE_2M)
	{
		// There are no ZONE_STD 4K pages available anymore, so we split this 2M page into 4K pages
		// Just use the 2M page's first 4K as the next PMM stack page
		// NOTE: After calling stack_switch() stackTop will point to the new entry, therefore setting stackTop->next is correct
		stackTop->next = stack_switch(SIZE_4K, zone, addr);
		stackTop->count = 0;

		// Mark the remaining 511 4K pages as free
		for(uintptr_t inner_addr = addr + FRAME_SIZE; inner_addr < addr + FRAME_SIZE_2M; inner_addr += FRAME_SIZE)
			_pmm_free(SIZE_4K, get_zone(SIZE_4K, inner_addr), inner_addr);
	}
	else if(size == SIZE_1G)
	{
		// There are no ZONE_STD 4K pages available anymore, so we split this 1G page into 2M and 4K pages
		// Just use the 1G page's first 4K as the next PMM stack page
		// NOTE: After calling stack_switch() stackTop will point to the new entry, therefore setting stackTop->next is correct
		stackTop->next = stack_switch(SIZE_4K, zone, addr);
		stackTop->count = 0;

		// Mark the following 511 4K pages as free
		for(uintptr_t inner_addr = addr + FRAME_SIZE; inner_addr < addr + FRAME_SIZE_2M; inner_addr += FRAME_SIZE)
			_pmm_free(SIZE_4K, get_zone(SIZE_4K, inner_addr), inner_addr);
		
		// Mark the remaining 511 2M pages as free
		for(uintptr_t inner_addr = addr + FRAME_SIZE_2M; inner_addr < addr + FRAME_SIZE_1G; inner_addr += FRAME_SIZE_2M)
			_pmm_free(SIZE_2M, get_zone(SIZE_2M, inner_addr), inner_addr);
	}
}

// Pushes a range of pages between (aligned) addresses start...end with the given size onto the respective stack.
static void pmm_push_range(uintptr_t start, uintptr_t end, int size)
{
	// Run through addresses in the given range
	uintptr_t inc = 0;
	switch(size)
	{
		case SIZE_4K:
			inc = FRAME_SIZE;
			break;

		case SIZE_2M:
			inc = FRAME_SIZE_2M;
			break;

		case SIZE_1G:
			inc = FRAME_SIZE_1G;
			break;
	}
	for(uintptr_t addr = start; addr < end; addr += inc)
	{
		// Add this address to the matching stack
		int zone = get_zone(size, addr);
		_pmm_free(size, zone, addr);
		
		// Remember page count
		int idx = SZ_TO_IDX(size, zone);
		pmm_counts[idx]++;
	}
}

void pmm_init(list_t *map)
{
	// First load the nine initial PMM stack pages into virtual memory by writing their physical address into the PMM PML1 page table
	// These stack pages are then referred twice by virtual memory (once in the kernel image address space, and here)
	for(int size = 0; size < SIZE_COUNT; size++)
	{
		for(int zone = 0; zone < ZONE_COUNT; zone++)
		{
			// Load stack page
			int idx = SZ_TO_IDX(size, zone);
			stack_switch(size, zone, (uintptr_t) &pmm_phy_stack[idx] - VM_KERNEL_IMAGE);
			
			// Clear page data
			memset(&pmm_stack[idx], 0, sizeof(*pmm_stack));
		}
	}

	list_for_each(map, node)
	{
		mm_map_entry_t *entry = container_of(node, mm_map_entry_t, node);

		// Get respective maximum address ranges for aligned pages with given sizes
		uintptr_t start = PAGE_ALIGN(entry->addr_start); // (finds the next page aligned address >= x)
		uintptr_t end = PAGE_ALIGN_REVERSE(entry->addr_end + 1); // (Finds the last page aligned address <= x); addr_end is inclusive, end should be exclusive, therefore we do +1

		uintptr_t start_2m = PAGE_ALIGN_2M(entry->addr_start);
		uintptr_t end_2m = PAGE_ALIGN_REVERSE_2M(entry->addr_end + 1);

		uintptr_t start_1g = PAGE_ALIGN_1G(entry->addr_start);
		uintptr_t end_1g = PAGE_ALIGN_REVERSE_1G(entry->addr_end + 1);

		// Fill physical address space with biggest possible (aligned) pages:
		// unused | 4K ... 4K | 2M ... 2M | 1G ... 1G | 2M ... 2M | 4K ... 4K | unused
		if(start_1g <= end_1g)
		{
			if(start <= start_2m)
				pmm_push_range(start, start_2m, SIZE_4K);

			if(end_2m <= end)
				pmm_push_range(end_2m, end, SIZE_4K);

			if(start_2m <= start_1g)
				pmm_push_range(start_2m, start_1g, SIZE_2M);

			if(end_1g <= end_2m)
				pmm_push_range(end_1g, end_2m, SIZE_2M);

			pmm_push_range(start_1g, end_1g, SIZE_1G);
		}
		else if(start_2m <= end_2m)
		{
			if(start <= start_2m)
				pmm_push_range(start, start_2m, SIZE_4K);

			if(end_2m <= end)
				pmm_push_range(end_2m, end, SIZE_4K);

			pmm_push_range(start_2m, end_2m, SIZE_2M);
		}
		else if(start <= end)
		{
			pmm_push_range(start, end, SIZE_4K);
		}
	}

	for(int zone = 0; zone < ZONE_COUNT; zone++)
	{
		for(int size = 0; size < SIZE_COUNT; size++)
		{
			int idx = SZ_TO_IDX(size, zone);
			uint64_t count = pmm_counts[idx];
			if(count > 0)
			{
				const char *zone_str = get_zone_str(zone);
				const char *size_str = get_size_str(size);
				trace_printf(" => Zone %s Size %s: %d frames\n", zone_str, size_str, count);
			}
		}
	}
}

uintptr_t pmm_alloc(void)
{
	return pmm_allocsz(SIZE_4K, ZONE_STD);
}

uintptr_t pmm_allocs(int size)
{
	return pmm_allocsz(size, ZONE_STD);
}

uintptr_t pmm_allocz(int zone)
{
	return pmm_allocsz(SIZE_4K, zone);
}

uintptr_t pmm_allocsz(int size, int zone)
{
	spin_lock(&pmm_lock);
	uintptr_t addr = _pmm_alloc(size, zone);
	spin_unlock(&pmm_lock);
	return addr;
}

void pmm_free(uintptr_t addr)
{
	pmm_frees(SIZE_4K, addr);
}

void pmm_frees(int size, uintptr_t addr)
{
	spin_lock(&pmm_lock);
	_pmm_free(size, get_zone(size, addr), addr);
	spin_unlock(&pmm_lock);
}

uintptr_t pmm_alloc_contiguous(int size, int count)
{
	spin_lock(&pmm_lock);
	
	
	
	spin_unlock(&pmm_lock);
}
