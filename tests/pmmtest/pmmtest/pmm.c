
#include "pmm.h"
#include "align.h"
#include "common.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// Pointer to the PMM PML1 (loops once in PML4).
#define PAGE_TABLE_OFFSET 0xFFFFFF7F7F7FF000

// The amount of size/zone stacks.
#define STACK_COUNT (ZONE_COUNT * SIZE_COUNT)

// The amount of addresses contained in one stack page.
#define PMM_STACK_PAGE_SIZE (PAGE_TABLE_ENTRY_COUNT - 2)

// Converts a size/zone pair to a stack ID.
#define SZ_TO_IDX(s,z) ((s) * ZONE_COUNT + (z))

// Structure of the internal stack pages.
#pragma pack(push, 1)
struct pmm_stack_page_s
{
    // Points to the next bottom stack page; 0 for the lowest stack page.
    // We assume that there is no physical address == 0x00000000.
    uint64_t next;

    // Amount of frames saved in this stack page.
    uint64_t count;

    // Frames saved in this stack page.
    uint64_t frames[PMM_STACK_PAGE_SIZE];
};
#pragma pack(pop)
typedef struct pmm_stack_page_s pmm_stack_page_t;

// Structure of freed address list pages in the defragmentation procedure.
#define PMM_DEFRAGMENT_FREED_LIST_ENTRY_COUNT 455
#pragma pack(push, 1)
struct pmm_defragment_freed_list_s
{
    // The memory block sizes of the respective stored address entries.
    uint8_t addressSizes[PMM_DEFRAGMENT_FREED_LIST_ENTRY_COUNT]; // 455 bytes

    // Unused, for alignment.
    uint8_t _align; // 1 byte

    // The memory block addresses.
    uint64_t addresses[PMM_DEFRAGMENT_FREED_LIST_ENTRY_COUNT]; // 3640 bytes
};
#pragma pack(pop)
typedef struct pmm_defragment_freed_list_s pmm_defragment_freed_list_t;

// Pointer to the current stack
// TODO these are page table entries later
#define PMM_AUX_PAGES_COUNT 64 // Auxiliary pages for bookkeeping in defragmentation algorithm; this value is safe for ~24 GiB physical RAM
static pmm_stack_page_t *pmmData[STACK_COUNT + 1 + PMM_AUX_PAGES_COUNT];

// Cache variable to quickly determine whether there are still pages of the given kind available.
static uint64_t pmmTotalFrameCounts[STACK_COUNT];

static uint64_t baseAddrOffset;
static uint64_t total4kPages;

// Determines whether the stack page list has already been initialized.
static bool stackPageListInitialized = false;

// Forward declarations
static void _pmm_free(int size, int zone, uintptr_t addr);


static const char *get_zone_str(int zone)
{
    switch(zone)
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
    switch(size)
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
    switch(size)
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

    uintptr_t vaddr = addr + baseAddrOffset;
    uintptr_t oldaddr = (uintptr_t)pmmData[idx] - baseAddrOffset;
    pmmData[idx] = vaddr;

    return oldaddr;
}

// Tries to reserve the given address as stack page address.
// Sorting is achieved by searching for a fitting insertion point and moving all smaller entries.
static bool _pmm_try_reserve_as_stack_page(uintptr_t addr)
{
    // Wait for proper initialization
    if(!stackPageListInitialized)
        return false;

    // Space left?
    pmm_stack_page_t *stackPageList = pmmData[STACK_COUNT];
    uintptr_t removeAddress = 0;
    if(stackPageList->count == PMM_STACK_PAGE_SIZE)
    {
        // Is there a lower address that could be removed?
        if(stackPageList->frames[stackPageList->count - 1] < addr)
        {
            removeAddress = stackPageList->frames[stackPageList->count - 1];
            --stackPageList->count;
        }
        else
            return false;
    }

    // Determine insertion point
    int index = 0;
    for(; index < (int)stackPageList->count; ++index)
        if(stackPageList->frames[index] < addr)
            break;

    // Move all smaller entries
    for(int i = stackPageList->count - 1; i >= index; --i)
        stackPageList->frames[i + 1] = stackPageList->frames[i];

    // Insert new entry
    stackPageList->frames[index] = addr;
    ++stackPageList->count;

    // Free removed address, if there is any
    if(removeAddress)
        _pmm_free(SIZE_4K, get_zone(SIZE_4K, removeAddress), removeAddress);
    return true;
}

// Tries to retrieve a stack page address from the high stack page list.
static uintptr_t _pmm_acquire_stack_page()
{
    // Addresses available?
    pmm_stack_page_t *stackPageList = pmmData[STACK_COUNT];
    if(stackPageList->count > 0)
    {
        // Return highest address -> move all entries one step
        uintptr_t highestAddress = stackPageList->frames[0];
        for(int i = 1; i < (int)stackPageList->count; ++i)
            stackPageList->frames[i - 1] = stackPageList->frames[i];
        --stackPageList->count;
        return highestAddress;
    }

    // No addresses available
    return 0;
}

// Retrieve an address with the given size/zone from a matching stack.
static uintptr_t _pmm_alloc(int size, int zone, int maxZone, int *effectiveZone)
{
    // Determine stack for the given size/zone combination
    int idx = SZ_TO_IDX(size, zone);
    pmm_stack_page_t *stackTop = pmmData[idx];

    // Still addresses available on that stack page? -> Just return the top most one of them
    if(stackTop->count != 0)
    {
        *effectiveZone = zone;
        return stackTop->frames[--stackTop->count];
    }

    // Stack page is used up, is there another one?
    if(stackTop->next)
    {
        // Go to next stack page
        uintptr_t addr = stack_switch(size, zone, stackTop->next);

        // Add the discarded page back to the stack page list?
        if(!_pmm_try_reserve_as_stack_page(addr))
        {
            // There seems to be no need for stack pages now
            // If the size and zone match, we can recycle the former stack page
            int stack_zone = get_zone(SIZE_4K, addr);
            if(size == SIZE_4K && stack_zone <= zone)
            {
                *effectiveZone = stack_zone;
                return addr;
            }
            else
            {
                // We cannot recycle this page, so free it
                _pmm_free(SIZE_4K, stack_zone, addr);
            }
        }

        // Run the allocation function again (this time with a filled stack page)
        return _pmm_alloc(size, zone, zone, effectiveZone);
    }

    // Try to allocate from a smaller zone
    if(zone > ZONE_DMA)
        return _pmm_alloc(size, zone - 1, maxZone, effectiveZone);

    // No pages of the desired size available anymore, so unfortunately we need to split a 2M or 1G page
    if(size == SIZE_2M)
    {
        // Allocate a fresh 1G page from the highest possible zone (its address is removed from the respective stack)
        int allocZone;
        uintptr_t addr = _pmm_alloc(SIZE_1G, maxZone, maxZone, &allocZone);
        if(addr)
        {
            // Mark last 511 2M pages of the 1G page as free
            for(uintptr_t off = FRAME_SIZE_2M; off < FRAME_SIZE_1G; off += FRAME_SIZE_2M)
                _pmm_free(SIZE_2M, allocZone, addr + off);

            // Return the first 2M page
            *effectiveZone = allocZone;
            return addr;
        }
    }
    else if(size == SIZE_4K)
    {
        // Allocate a fresh 2M page from the highest possible zone (its address is removed from the respective stack)
        int allocZone;
        uintptr_t addr = _pmm_alloc(SIZE_2M, maxZone, maxZone, &allocZone);
        if(addr)
        {
            // Mark last 511 4K pages of the 2M page as free
            for(uintptr_t off = FRAME_SIZE; off < FRAME_SIZE_2M; off += FRAME_SIZE)
                _pmm_free(SIZE_4K, allocZone, addr + off);

            // Return the first 4K page
            *effectiveZone = allocZone;
            return addr;
        }
    }

    // Well, looks like RAM is completely used up (or terribly fragmented)
    return 0;
}

// Adds the given address back to its matching stack.
static void _pmm_free(int size, int zone, uintptr_t addr)
{
    // Reserve high 4K pages as future stack pages
    if(size == SIZE_4K && _pmm_try_reserve_as_stack_page(addr))
        return;

    // Get top most matching stack page
    int idx = SZ_TO_IDX(size, zone);
    pmm_stack_page_t **stackTop = &pmmData[idx];

    // Still space left on that stack page -> store addr there
    if((*stackTop)->count != PMM_STACK_PAGE_SIZE)
    {
        (*stackTop)->frames[(*stackTop)->count++] = addr;
        return;
    }

    // If this is a 4K ZONE_STD page, just re-use it as the next 4K/ZONE_STD PMM stack page
    if(size == SIZE_4K && zone == ZONE_STD)
    {
        (*stackTop)->next = stack_switch(size, zone, addr);
        (*stackTop)->count = 0;
        return;
    }

    // Allocate new physical stack page for size/zone
    int allocZone;
    uintptr_t new_addr = _pmm_acquire_stack_page();
    if(!new_addr)
        new_addr = _pmm_alloc(SIZE_4K, ZONE_STD, ZONE_STD, &allocZone);
    if(new_addr)
    {
        // Add the freed address to the current stack page
        (*stackTop)->next = stack_switch(size, zone, new_addr);
        (*stackTop)->count = 0;
        (*stackTop)->frames[(*stackTop)->count++] = addr;
        return;
    }

    // Allocation failed (probably physical RAM too small)
    if(size == SIZE_4K)
    {
        // This is a 4K page in an arbitrary zone, just use it
        (*stackTop)->next = stack_switch(size, zone, addr);
        (*stackTop)->count = 0;
    }
    else if(size == SIZE_2M)
    {
        // There are no ZONE_STD 4K pages available anymore, so we split this 2M page into 4K pages,
        // and use the 2M page's first 4K as the next PMM stack page to map the 511 new 4K pages
        (*stackTop)->next = stack_switch(SIZE_4K, zone, addr);
        (*stackTop)->count = 0;

        // Mark the remaining 511 4K pages as free
        for(uintptr_t inner_addr = addr + FRAME_SIZE; inner_addr < addr + FRAME_SIZE_2M; inner_addr += FRAME_SIZE)
            _pmm_free(SIZE_4K, get_zone(SIZE_4K, inner_addr), inner_addr);
    }
    else if(size == SIZE_1G)
    {
        // There are no ZONE_STD 4K pages available anymore, so we split this 1G page into 2M and 4K pages,
        // and use the 2M page's first 4K as the next PMM stack page to map the new smaller pages
        (*stackTop)->next = stack_switch(SIZE_4K, zone, addr);
        (*stackTop)->count = 0;

        // Mark the following 511 4K pages as free
        for(uintptr_t inner_addr = addr + FRAME_SIZE; inner_addr < addr + FRAME_SIZE_2M; inner_addr += FRAME_SIZE)
            _pmm_free(SIZE_4K, get_zone(SIZE_4K, inner_addr), inner_addr);

        // Mark the remaining 511 2M pages as free
        for(uintptr_t inner_addr = addr + FRAME_SIZE_2M; inner_addr < addr + FRAME_SIZE_1G; inner_addr += FRAME_SIZE_2M)
            _pmm_free(SIZE_2M, get_zone(SIZE_2M, inner_addr), inner_addr);
    }
}

// Reads an arbitrary stack index, using the given stack page address list.
static uint64_t _pmm_defragment_read_index(uint64_t *stackPageAddrList, int size, int zone, int index, int firstPageFrameCount, uint64_t *currentStackPageAddrPtr)
{
    // Load correct stack page
    int stackPageIndex;
    int firstStackPageEmptyIndicesCount = PMM_STACK_PAGE_SIZE - firstPageFrameCount;
    if(index < firstPageFrameCount)
    {
        stackPageIndex = firstPageFrameCount - 1 - index;
        if(*currentStackPageAddrPtr != stackPageAddrList[0])
        {
            stack_switch(size, zone, stackPageAddrList[0]);
            *currentStackPageAddrPtr = stackPageAddrList[0];
        }
    }
    else
    {
        int stackPageAddrListIndex = (index + firstStackPageEmptyIndicesCount) / PMM_STACK_PAGE_SIZE;
        stackPageIndex = PMM_STACK_PAGE_SIZE - 1 - (index - firstPageFrameCount) % PMM_STACK_PAGE_SIZE;
        if(*currentStackPageAddrPtr != stackPageAddrList[stackPageAddrListIndex])
        {
            stack_switch(size, zone, stackPageAddrList[stackPageAddrListIndex]);
            *currentStackPageAddrPtr = stackPageAddrList[stackPageAddrListIndex];
        }
    }
    return pmmData[SZ_TO_IDX(size, zone)]->frames[stackPageIndex];
}

// Writes at an arbitrary stack index, using the given stack page address list.
static void _pmm_defragment_write_index(uint64_t *stackPageAddrList, int size, int zone, int index, int firstPageFrameCount, uint64_t *currentStackPageAddrPtr, uint64_t value)
{
    // Load correct stack page
    int stackPageIndex;
    int firstStackPageEmptyIndicesCount = PMM_STACK_PAGE_SIZE - firstPageFrameCount;
    if(index < firstPageFrameCount)
    {
        stackPageIndex = firstPageFrameCount - 1 - index;
        if(*currentStackPageAddrPtr != stackPageAddrList[0])
        {
            stack_switch(size, zone, stackPageAddrList[0]);
            *currentStackPageAddrPtr = stackPageAddrList[0];
        }
    }
    else
    {
        int stackPageAddrListIndex = (index + firstStackPageEmptyIndicesCount) / PMM_STACK_PAGE_SIZE;
        stackPageIndex = PMM_STACK_PAGE_SIZE - 1 - (index - firstPageFrameCount) % PMM_STACK_PAGE_SIZE;
        if(*currentStackPageAddrPtr != stackPageAddrList[stackPageAddrListIndex])
        {
            stack_switch(size, zone, stackPageAddrList[stackPageAddrListIndex]);
            *currentStackPageAddrPtr = stackPageAddrList[stackPageAddrListIndex];
        }
    }
    pmmData[SZ_TO_IDX(size, zone)]->frames[stackPageIndex] = value;
}

// Builds a min heap for defragmentation at the given stack index, using the given stack page address list.
static void _pmm_defragment_min_heapify(uint64_t *stackPageAddrList, int size, int zone, int index, int totalFrameCount, int firstStackPageFrameCount, uint64_t *currentStackPageAddrPtr)
{
    // Iterative MIN-HEAPIFY
    int i = index;
    while(i < totalFrameCount)
    {
        // Calculate indices of child nodes
        int left = 2 * i + 1;
        int right = 2 * i + 2;

        // Get smallest index of the current node and its children
        int smallest = i;
        uint64_t rootValue = _pmm_defragment_read_index(stackPageAddrList, size, zone, i, firstStackPageFrameCount, currentStackPageAddrPtr);
        uint64_t smallestValue = rootValue;
        uint64_t leftValue = 0;
        uint64_t rightValue = 0;
        if(left < totalFrameCount)
        {
            // Compare root node and left child
            leftValue = _pmm_defragment_read_index(stackPageAddrList, size, zone, left, firstStackPageFrameCount, currentStackPageAddrPtr);
            if(rootValue > leftValue)
            {
                // Left is the smaller value
                smallest = left;
                smallestValue = leftValue;
            }
        }
        if(right < totalFrameCount)
        {
            // Compare root node and right child
            rightValue = _pmm_defragment_read_index(stackPageAddrList, size, zone, right, firstStackPageFrameCount, currentStackPageAddrPtr);
            if(smallestValue > rightValue)
            {
                // Right is the smallest value
                smallest = right;
                smallestValue = rightValue;
            }
        }

        // Swap root node and smallest node, if they differ
        if(smallest != i)
        {
            _pmm_defragment_write_index(stackPageAddrList, size, zone, smallest, firstStackPageFrameCount, currentStackPageAddrPtr, rootValue);
            _pmm_defragment_write_index(stackPageAddrList, size, zone, i, firstStackPageFrameCount, currentStackPageAddrPtr, smallestValue);
            i = smallest;
        }
        else
            break;
    }
}

// Frees empty stack pages.
// NOTE: This function uses the left side of the auxiliary management data section!
static void _pmm_defragment_free_empty_stack_pages()
{
    // Create a list of all empty stack pages
    int emptyStackPagesCount = 0;
    uint64_t *emptyStackPagesList = (uint64_t *)pmmData[STACK_COUNT + 1];
    for(int size = 0; size < SIZE_COUNT; ++size)
    {
        // Begin with ZONE_STD
        for(int zone = ZONE_COUNT - 1; zone >= 0; --zone)
        {
            // Determine stack for the given size/zone combination
            int idx = SZ_TO_IDX(size, zone);
            pmm_stack_page_t **stackTop = &pmmData[idx];

            // Stack page empty and not the last one?
            while((*stackTop)->count == 0 && (*stackTop)->next)
            {
                // Go to next stack page
                emptyStackPagesList[emptyStackPagesCount++] = stack_switch(size, zone, (*stackTop)->next);
            }
        }
    }

    // Free all empty stack pages
    for(int i = 0; i < emptyStackPagesCount; ++i)
    {
        // Add the discarded page back to the stack page list?
        uint64_t addr = emptyStackPagesList[i];
        printf("Free: %016x\n", addr);
        if(!_pmm_try_reserve_as_stack_page(addr))
        {
            // We cannot recycle this page, so free it
            _pmm_free(SIZE_4K, get_zone(SIZE_4K, addr), addr);
        }
    }
}

// Tries do defragment and reorganize physical memory. Warning: This is a VERY expensive operation!
// After running this method the address stacks are guaranteed to be sorted in descending order.
// This method calls itself two times again, once to merge remaining 2M pages and once to perform a final sorting.
// - Sort all stacks using in-place Heapsort.
// - (TODO Move each stack page to an unused 4K page at a highest possible address.)
// - If desired: Merge pages to 2M and 1G pages as much as possible
static void _pmm_defragment(bool merge, bool secondPass)
{
    // Initialize temporary storage for freed addresses
    // Grow from right to left, to avoid intersecting the stack page address list that is stored from left to right
    pmm_defragment_freed_list_t *firstFreedEntriesListPage = (pmm_defragment_freed_list_t *)pmmData[STACK_COUNT + 1 + PMM_AUX_PAGES_COUNT - 1];
    pmm_defragment_freed_list_t *currentFreedEntriesListPage = firstFreedEntriesListPage;
    int freedEntriesCount = 0;

    // Sort stacks for all zones and sizes
    bool first = true;
    for(int size = 0; size < SIZE_COUNT; ++size)
    {
        // Begin with ZONE_STD
        for(int zone = ZONE_COUNT - 1; zone >= 0; --zone)
        {
            // Get related stack page
            int idx = SZ_TO_IDX(size, zone);
            pmm_stack_page_t **stackTop = &pmmData[idx];
            if(!(*stackTop) || (*stackTop)->count < 2)
                continue; // Nothing to do here

            // Traverse stack to get physical addresses of all stack pages
            int stackPageCount = 0;
            int totalFrameCount = 0;
            int firstStackPageFrameCount = (stackTop ? (*stackTop)->count : 0);
            uint64_t *stackPageAddrList = (uint64_t *)pmmData[STACK_COUNT + 1];
            while(*stackTop)
            {
                // Add amount of free pages
                totalFrameCount += (*stackTop)->count;

                // Store address of stack page
                bool nextStackPageExists = (*stackTop)->next ? true : false;
                uint64_t curStackPageAddr = stack_switch(size, zone, (*stackTop)->next);
                stackPageAddrList[stackPageCount] = curStackPageAddr;
                ++stackPageCount;

                // Next page valid?
                if(!nextStackPageExists)
                    break;
            }

            // Restore top stack page
            stack_switch(size, zone, stackPageAddrList[0]);

            // Run through addresses and build a min heap (this way the array can be sorted *descending* in the next step)
            uint64_t currentStackPageAddr = stackPageAddrList[0];
            for(int i = totalFrameCount / 2 - 1; i >= 0; --i)
                _pmm_defragment_min_heapify(stackPageAddrList, size, zone, i, totalFrameCount, firstStackPageFrameCount, &currentStackPageAddr);

            // Build sorted array
            int cnt = totalFrameCount;
            for(int i = totalFrameCount - 1; i >= 1; --i)
            {
                // Swap arr[0] and arr[i]
                uint64_t rootVal = _pmm_defragment_read_index(stackPageAddrList, size, zone, 0, firstStackPageFrameCount, &currentStackPageAddr);
                uint64_t bottomVal = _pmm_defragment_read_index(stackPageAddrList, size, zone, i, firstStackPageFrameCount, &currentStackPageAddr);
                _pmm_defragment_write_index(stackPageAddrList, size, zone, 0, firstStackPageFrameCount, &currentStackPageAddr, bottomVal);
                _pmm_defragment_write_index(stackPageAddrList, size, zone, i, firstStackPageFrameCount, &currentStackPageAddr, rootVal);

                // Update remaining tree
                --cnt;
                _pmm_defragment_min_heapify(stackPageAddrList, size, zone, 0, cnt, firstStackPageFrameCount, &currentStackPageAddr);
            }

            // TODO Move stack pages to high addresses, since some might be left within splitted 2M/1G blocks that were already freed again
            //      this needs to be done within the 4K loops between sorting and merging

            // Sorting is done, now merge pages
            if(!merge)
                continue; // Next zone/size

            // Compute alignment and size parameters
            uint64_t alignment = FRAME_SIZE_2M;
            if(size == SIZE_2M)
                alignment = FRAME_SIZE_1G;
            uint64_t alignmentMask = alignment - 1;
            uint64_t sizeBytes = FRAME_SIZE;
            if(size == SIZE_2M)
                sizeBytes = FRAME_SIZE_2M;

            // Run through stack; since the addresses are stored in descending order, we can identify contiguous blocks in linear time now
            uint64_t lastAlignedEndAddress = 0xFFFFFFFF; // Invalid value
            uint64_t lastContiguousAddress = 0xFFFFFFFF; // Invalid value
            int removedPagesCountTotal = 0;
            for(int i = 0; i < totalFrameCount; ++i)
            {
                // Get next address
                // Is its end aligned to a 2M/1G boundary? Does it belong to a contiguous section?
                uint64_t addr = _pmm_defragment_read_index(stackPageAddrList, size, zone, i, firstStackPageFrameCount, &currentStackPageAddr);
                if(!((addr + sizeBytes) & alignmentMask))
                {
                    // This block is aligned
                    lastAlignedEndAddress = addr;
                    lastContiguousAddress = addr;
                }
                else if(lastContiguousAddress - sizeBytes == addr)
                {
                    // This address is contiguous
                    lastContiguousAddress = addr;

                    // Full block?
                    if((lastAlignedEndAddress + sizeBytes) - lastContiguousAddress == alignment)
                    {
                        // Do not merge the first 2M block, to keep enough 4K pages
                        if(first && SIZE_2M)
                        {
                            // Clear flag and look for the next block
                            first = false;
                            lastAlignedEndAddress = 0xFFFFFFFF; // Invalid value
                            continue; // for loop
                        }

                        // Remove pages from stack by moving the upper addresses down by 512 steps (ratio of FRAME_SIZE and FRAME_SIZE_2M)
                        for(int j = i - 512; j >= removedPagesCountTotal; --j)
                        {
                            // Overwrite entry
                            uint64_t tmp = _pmm_defragment_read_index(stackPageAddrList, size, zone, j, firstStackPageFrameCount, &currentStackPageAddr);
                            _pmm_defragment_write_index(stackPageAddrList, size, zone, j + 512, firstStackPageFrameCount, &currentStackPageAddr, tmp);
                        }

                        // Update top three stack pages
                        int removedPagesCount = 512;
                        removedPagesCountTotal += removedPagesCount;
                        stack_switch(size, zone, stackPageAddrList[0]);
                        while(true)
                        {
                            // Subtract from frame count
                            if((int)(*stackTop)->count < removedPagesCount)
                            {
                                // This stack page is cleared completely, it will be freed later
                                removedPagesCount -= (*stackTop)->count;
                                (*stackTop)->count = 0;
                            }
                            else
                            {
                                // This stack page keeps some frames, or is cleared completely (if removedPagesCount == count)
                                (*stackTop)->count -= removedPagesCount;
                                removedPagesCount = 0;
                                break;
                            }

                            // Next stack page (this line is executed at least once)
                            currentStackPageAddr = (*stackTop)->next;
                            stack_switch(size, zone, currentStackPageAddr);
                        }

                        // Add the new page to the "freed" list
                        int freedEntriesListIndex = freedEntriesCount % PMM_DEFRAGMENT_FREED_LIST_ENTRY_COUNT;
                        if(freedEntriesListIndex == 0 && freedEntriesCount > 0)
                            --currentFreedEntriesListPage;
                        currentFreedEntriesListPage->addressSizes[freedEntriesListIndex] = size + 1; // 4K -> 2M, 2M -> 1G
                        currentFreedEntriesListPage->addresses[freedEntriesListIndex] = lastContiguousAddress;
                        ++freedEntriesCount;
                    }
                }
                else
                    lastAlignedEndAddress = 0xFFFFFFFF; // Invalid value
            }

            // Restore top stack page
            stack_switch(size, zone, stackPageAddrList[0]);
        }
    }

    // Free pages that merging enqueued as "freed"
    if(!merge)
        return;

    // Free empty stack pages
    _pmm_defragment_free_empty_stack_pages();

    // Free merged pages
    currentFreedEntriesListPage = firstFreedEntriesListPage;
    for(int i = 0; i < freedEntriesCount; ++i)
    {
        int freedEntriesListIndex = i % PMM_DEFRAGMENT_FREED_LIST_ENTRY_COUNT;
        if(freedEntriesListIndex == 0 && i > 0)
            --currentFreedEntriesListPage;
        uint8_t size = currentFreedEntriesListPage->addressSizes[freedEntriesListIndex];
        uint64_t address = currentFreedEntriesListPage->addresses[freedEntriesListIndex];
        printf("Freed: %s  %016x\n", (size == SIZE_2M ? "2M" : "1G"), address);
        _pmm_free(size, get_zone(size, address), address);
    }

    // Which defragmentation pass is running?
    if(secondPass)
    {
        // Second merge is done, just do a final sorting round
        _pmm_defragment(false, false);
    }
    else
    {
        // Do another round to merge remaining 2M pages
        _pmm_defragment(true, true);
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

        // Keep track of free page count
        int idx = SZ_TO_IDX(size, zone);
        pmmTotalFrameCounts[idx]++;
    }
}

void pmm_init(uint64_t addr_start, uint64_t addr_end, uint64_t baseOffset, uintptr_t phyStack)
{
    baseAddrOffset = baseOffset;

    // First load the nine initial PMM stack pages into virtual memory by writing their physical address into the PMM PML1 page table
    // These stack pages are then referred twice by virtual memory (once in the kernel image address space, and here)
    for(int size = 0; size < SIZE_COUNT; size++)
    {
        for(int zone = 0; zone < ZONE_COUNT; zone++)
        {
            // Load stack page
            int idx = SZ_TO_IDX(size, zone);
            stack_switch(size, zone, (uintptr_t)&(((pmm_stack_page_t *)phyStack)[idx]));

            // Clear page data
            memset(pmmData[idx], 0, sizeof(**pmmData));
        }
    }

    pmmData[9] = baseAddrOffset + (uint64_t)&(((pmm_stack_page_t *)phyStack)[9]);
    pmmData[9]->count = 0;
    pmmData[9]->next = 0; // Unused

    total4kPages = (PAGE_ALIGN_REVERSE(addr_end + 1) - PAGE_ALIGN(phyStack)) / FRAME_SIZE;
    printf("Total 4K pages: %d\n", total4kPages);

    // Get respective maximum address ranges for aligned pages with given sizes
    uintptr_t start = PAGE_ALIGN(addr_start); // (finds the next page aligned address >= x)
    uintptr_t end = PAGE_ALIGN_REVERSE(addr_end + 1); // (Finds the last page aligned address <= x); addr_end is inclusive, end should be exclusive, therefore we do +1

    // Reserve some high 4K addresses as auxiliary memory
    // TODO this will be moved to the end; we need contiguous memory in our testing buffer here, but in the real PMM this is done by virtual memory automatically
    end -= PMM_AUX_PAGES_COUNT * FRAME_SIZE;
    for(int i = 0; i < PMM_AUX_PAGES_COUNT; ++i)
        pmmData[10 + i] = baseOffset + end + i * 4096;

    uintptr_t start_2m = PAGE_ALIGN_2M(addr_start);
    uintptr_t end_2m = PAGE_ALIGN_REVERSE_2M(addr_end + 1);

    uintptr_t start_1g = PAGE_ALIGN_1G(addr_start);
    uintptr_t end_1g = PAGE_ALIGN_REVERSE_1G(addr_end + 1);

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

    /*for(int zone = 0; zone < ZONE_COUNT; zone++)
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
    }*/

    // Reserve some high 4K addresses for the stack page collection
    // Right now all addresses should sorted in descending order, so we should get the highest available pages
    pmm_stack_page_t *stackPageList = pmmData[9];
    int tmp;
    for(int i = 0; i < PMM_STACK_PAGE_SIZE; ++i)
        stackPageList->frames[i] = _pmm_alloc(SIZE_4K, ZONE_STD, ZONE_STD, &tmp);
    stackPageList->count = PMM_STACK_PAGE_SIZE;
    stackPageListInitialized = true;
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
    int allocZone;
    uintptr_t addr = _pmm_alloc(size, zone, zone, &allocZone);
    return addr;
}

void pmm_free(uintptr_t addr)
{
    pmm_frees(SIZE_4K, addr);
}

void pmm_frees(int size, uintptr_t addr)
{
    _pmm_free(size, get_zone(size, addr), addr);
}

uintptr_t pmm_alloc_contiguous(int size, int count)
{
    // The address stacks need to be sorted
    _pmm_defragment(true, false);

    // Compute byte size per block
    uint64_t sizeBytes = FRAME_SIZE;
    if(size == SIZE_2M)
        sizeBytes = FRAME_SIZE_2M;
    else if(size == SIZE_1G)
        sizeBytes = FRAME_SIZE_1G;

    // Run through zones and try to find a fitting memory block
    for(int zone = 0; zone < ZONE_COUNT; ++zone)
    {
        int idx = SZ_TO_IDX(size, zone);
        pmm_stack_page_t **stackTop = &pmmData[idx];

        // Traverse stack and store physical addresses of all involved stack pages (1:1 copy from _pmm_defragment)
        int stackPageCount = 0;
        int totalFrameCount = 0;
        int firstStackPageFrameCount = (stackTop ? (*stackTop)->count : 0);
        uint64_t *stackPageAddrList = (uint64_t *)pmmData[10];
        while(*stackTop)
        {
            // Add amount of free pages
            totalFrameCount += (*stackTop)->count;

            // Store address of stack page
            bool nextStackPageExists = (*stackTop)->next ? true : false;
            uint64_t curStackPageAddr = stack_switch(size, zone, (*stackTop)->next);
            stackPageAddrList[stackPageCount] = curStackPageAddr;
            ++stackPageCount;

            // Next page valid?
            // NOTE this is not needed in the real PMM later, since we add no base offset there
            if(!nextStackPageExists)
                break;
        }

        // Restore top stack page
        uint64_t currentStackPageAddr = stackPageAddrList[0];
        stack_switch(size, zone, currentStackPageAddr);

        // Try to find contiguous block by iterating backwards
        uint64_t lastEndAddress = 0xFFFFFFFF; // Invalid value
        uint64_t lastContiguousAddress = 0xFFFFFFFF; // Invalid value
        int contiguousPagesCount = 0;
        for(int i = 0; i < totalFrameCount; ++i)
        {
            // Get next address
            // Does it belong to a contiguous section?
            uint64_t addr = _pmm_defragment_read_index(stackPageAddrList, size, zone, i, firstStackPageFrameCount, &currentStackPageAddr);
            if(lastContiguousAddress - sizeBytes == addr)
            {
                // This address is contiguous
                lastContiguousAddress = addr;
                ++contiguousPagesCount;

                // Full block?
                if(contiguousPagesCount == count)
                {
                    printf("Found block: %016x - %016x\n", lastContiguousAddress, lastEndAddress + sizeBytes);

                    // Remove pages from stack by moving the upper addresses down by "count" steps
                    for(int j = i - contiguousPagesCount; j >= 0; --j)
                    {
                        // Overwrite entry
                        uint64_t tmp = _pmm_defragment_read_index(stackPageAddrList, size, zone, j, firstStackPageFrameCount, &currentStackPageAddr);
                        _pmm_defragment_write_index(stackPageAddrList, size, zone, j + contiguousPagesCount, firstStackPageFrameCount, &currentStackPageAddr, tmp);
                    }

                    // Update top stack pages
                    currentStackPageAddr = stackPageAddrList[0];
                    stack_switch(size, zone, currentStackPageAddr);
                    while(true)
                    {
                        // Subtract from frame count
                        if((int)(*stackTop)->count < contiguousPagesCount)
                        {
                            // This stack page is cleared completely, it will be freed later
                            contiguousPagesCount -= (*stackTop)->count;
                            (*stackTop)->count = 0;
                        }
                        else
                        {
                            // This stack page keeps some frames, or is cleared completely (if removedPagesCount == count)
                            (*stackTop)->count -= contiguousPagesCount;
                            contiguousPagesCount = 0;
                            break;
                        }

                        // Next stack page
                        currentStackPageAddr = (*stackTop)->next;
                        stack_switch(size, zone, currentStackPageAddr);
                    }

                    // Restore top stack page
                    stack_switch(size, zone, stackPageAddrList[0]);

                    // Free empty stack pages
                    _pmm_defragment_free_empty_stack_pages();

                    // Finished, return block
                    return lastContiguousAddress;
                }
            }
            else
            {
                // Start new try with the current address
                lastEndAddress = addr;
                lastContiguousAddress = addr;
                contiguousPagesCount = 1;
            }
        }

        // Restore top stack page
        stack_switch(size, zone, stackPageAddrList[0]);
    }

    // No contiguous block found
    return 0;
}

#define DUMP_FREE 0x01
#define DUMP_STACK_PAGE 0x02
#define DUMP_SIZE_2M 0x04
#define DUMP_SIZE_1G 0x08
#define DUMP_RESERVED 0x10
#define DUMP_AUX 0x20
void pmm_dump_stack(const char *name)
{
    printf("Dumping stacks...\n");
    uint64_t dataLen = 8 + total4kPages;
    uint8_t *data = calloc(dataLen, 1);
    *((uint64_t *)data) = total4kPages;

    for(int zone = 0; zone < ZONE_COUNT; zone++)
    {
        for(int size = 0; size < SIZE_COUNT; size++)
        {
            uint64_t sizeBytes = FRAME_SIZE;
            if(size == SIZE_2M)
                sizeBytes = FRAME_SIZE_2M;
            else if(size == SIZE_1G)
                sizeBytes = FRAME_SIZE_1G;

            int idx = SZ_TO_IDX(size, zone);
            uint64_t count = pmmTotalFrameCounts[idx];
            if(count > 0)
            {
                const char *zone_str = get_zone_str(zone);
                const char *size_str = get_size_str(size);
                printf("    => Zone %s Size %s: %d frames\n", zone_str, size_str, count);

                pmm_stack_page_t *curStack = pmmData[idx];
                while(curStack)
                {
                    printf("        StackPage at %#016x: %d frames\n", (uint64_t)curStack - baseAddrOffset, curStack->count);
                    for(int i = 0; i < curStack->count; ++i)
                    {
                        uint64_t dataIndex = curStack->frames[i] / 4096;

                        uint64_t flags = DUMP_FREE;
                        if(size == SIZE_2M)
                            flags |= DUMP_SIZE_2M;
                        else if(size == SIZE_1G)
                            flags |= DUMP_SIZE_1G;

                        for(int j = 0; j < sizeBytes / 4096; ++j)
                            data[8 + dataIndex + j] = flags;
                    }

                    data[8 + ((uint64_t)curStack - baseAddrOffset) / 4096] = DUMP_STACK_PAGE;

                    if(!curStack->next)
                        break;
                    curStack = baseAddrOffset + curStack->next;
                }
            }
        }
    }

    pmm_stack_page_t *stackPageList = pmmData[9];
    for(int i = 0; i < stackPageList->count; ++i)
        data[8 + stackPageList->frames[i] / 4096] = DUMP_RESERVED;

    for(int i = 0; i < PMM_AUX_PAGES_COUNT; ++i)
        data[8 + ((uint64_t)pmmData[10 + i] - baseAddrOffset) / 4096] = DUMP_AUX;

    FILE *f = fopen(name, "w");
    fwrite(data, 1, dataLen, f);
    fclose(f);

    printf("  Dump completed.\n\n");
}