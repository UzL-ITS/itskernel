
#include <mm/heap.h>
#include <lock/spinlock.h>
#include <mm/pmm.h>
#include <mm/vmm.h>
#include <mm/common.h>
#include <mm/align.h>
#include <mm/range.h>
#include <panic/panic.h>
#include <trace/trace.h>
#include <stdlib/assert.h>
#include <stdbool.h>
#include <stdint.h>

/* a magic id used to check ifrequests are valid */
#define HEAP_MAGIC 0x461E7B705515DB7F

// The tolerable amount of unused space on an allocated large page.
// Used for determining a good fit for contiguous allocations.
#define TOLERABLE_PAGE_SPACE_WASTE_1G 536870912 // 50%

/* the states a heap node can be in */
typedef enum
{
    HEAP_FREE,         /* not allocated */
    HEAP_RESERVED, /* allocated, physical frames not managed by us */
    HEAP_ALLOCATED /* allocated, physical frames managed by us */
} heap_state_t;

typedef struct heap_node
{
    struct heap_node *next;
    struct heap_node *prev;
    heap_state_t state;
    vm_acc_t flags;
    uintptr_t start; /* the address of the first byte, inclusive */
    uintptr_t end;     /* the address of the last byte, inclusive */
    uint64_t magic;
} heap_node_t;

static heap_node_t *heap_root;
static spinlock_t heap_lock = SPIN_UNLOCKED;

void heap_init(void)
{
    /* hard coded start of the kernel heap (inclusive) */
    uintptr_t heap_start = VM_HIGHER_HALF;

    /* hard coded end of the kernel heap (inclusive) */
    uintptr_t heap_end = PMM_INTERNAL_DATA_ADDRESS - 1;

    /* allocate some space forthe root node */
    uintptr_t root_phy = pmm_alloc();
    if(!root_phy)
        panic("couldn't allocate physical frame forheap root node");

    /* sanity check which probably seems completely ridiculous */
    if((heap_start + FRAME_SIZE) >= heap_end)
        panic("no room forheap");

    /* the root node will take the first virtual address */
    heap_root = (heap_node_t *) heap_start;
    if(!vmm_map(heap_start, root_phy, VM_R | VM_W))
        panic("couldn't map heap root node into the virtual memory");

    /* fill out the root node */
    heap_root->next = 0;
    heap_root->prev = 0;
    heap_root->state = HEAP_FREE;
    heap_root->start = heap_start + FRAME_SIZE;
    heap_root->end = heap_end;
    heap_root->magic = heap_root->start ^ HEAP_MAGIC;
}

static heap_node_t *find_node(size_t size, uint64_t alignment)
{
    // Look for the first node that will fit the requested size
    for(heap_node_t *node = heap_root; node; node = node->next)
    {
        // Skip non-free nodes
        if(node->state != HEAP_FREE)
            continue;
		
		// Calculate aligned start
		uint64_t nodeStart = node->start;
		uint64_t nodeEnd = node->end;
		uint64_t alignedStart = ((nodeStart + (alignment - 1)) & ~(alignment - 1)) - FRAME_SIZE; // FRAME_SIZE for the 4K node header block
		if(alignedStart + FRAME_SIZE + size > nodeEnd + 1)
			continue;
		
		// Spare space at the begin?
		if(alignedStart + FRAME_SIZE > nodeStart)
		{
			// Move node to alignedStart, just waste the remaining heap address space, it is still too much to ever exhaust it within the kernel
			uint64_t nodePhy = vmm_unmap((uint64_t)node);
			node = (heap_node_t *)alignedStart;
			if(vmm_map((uint64_t)node, nodePhy, VM_R | VM_W))
			{
				// Update predecessor and successor
				if(node->prev)
					node->prev->next = node;
				else
					heap_root = node;
				if(node->next)
					node->next->prev = node;
				
				// Update start address and magic value
				node->start = alignedStart + FRAME_SIZE;
				node->magic = node->start ^ HEAP_MAGIC;
			}
			else
				panic("Couldn't remap heap node to aligned position");
		}
		
		// Remaining space at the end?
        uint64_t remainingSize = (nodeEnd + 1) - (alignedStart + FRAME_SIZE + size);
        if(remainingSize >= (FRAME_SIZE * 2))
        {
            // Get a new physical page for the successor node
            uintptr_t nextNodePhy = pmm_alloc();
            if(nextNodePhy)
            {
                // Map new heap node into virtual memory
                heap_node_t *nextNode = (heap_node_t *)(alignedStart + FRAME_SIZE + size);
                if(vmm_map((uint64_t)nextNode, nextNodePhy, VM_R | VM_W))
                {
                    // Set limits of new and old node
					node->end = alignedStart + FRAME_SIZE + size - 1;
                    nextNode->start = alignedStart + FRAME_SIZE + size + FRAME_SIZE;
                    nextNode->end = nodeEnd;
                    nextNode->magic = nextNode->start ^ HEAP_MAGIC;
					
					// Set node state
                    nextNode->state = HEAP_FREE;
					
					// Update successor and predecessor
					nextNode->next = node->next;
					if(nextNode->next)
						nextNode->next->prev = nextNode;
					node->next = nextNode;
					nextNode->prev = node;
                }
                else
                    pmm_free(nextNodePhy);
            }
        }

        // Mark node as reserved (but not allocated yet)
        node->state = HEAP_RESERVED;
        return node;
    }

    return 0;
}

static void _heap_free(void *ptr)
{
    /* find where the node is */
    heap_node_t *node = (heap_node_t *)((uintptr_t) ptr - FRAME_SIZE);
	
	/* check if the magic matches to see if we get passed a dodgy pointer */
    assert(node->magic == (node->start ^ HEAP_MAGIC));

    /* free the physical frames if heap_alloc allocated them */
    size_t size = node->end - node->start + 1;
    if(node->state == HEAP_ALLOCATED)
        range_free(node->start, size);

    /* set the node's state to free */
    node->state = HEAP_FREE;

    /* try to coalesce with the next node */
    heap_node_t *next = node->next;
    if(next && next->state == HEAP_FREE && node->end + 1 == (uint64_t)next)
    {
        /* update the pointers */
        node->next = next->next;
        if(next->next)
            next->next->prev = node;

        /* update the address range */
        node->end = next->end;

        /* unmap and free the physical frame behind the next node */
        pmm_free(vmm_unmap((uintptr_t) next));
    }

    /* try to coalesce with the previous node */
    heap_node_t *prev = node->prev;
    if(prev && prev->state == HEAP_FREE && prev->end + 1 == (uint64_t)node)
    {
        /* update the pointers */
        prev->next = node->next;
        if(node->next)
            node->next->prev = prev;

        /* update the address range */
        prev->end = node->end;

        /* unmap and free the physical frame behind the next node */
        pmm_free(vmm_unmap((uintptr_t) next));
    }
}

// Sub function of _heap_alloc. Tries to allocate a contiguous block of pages with the given size.
static void *_heap_alloc_contiguous(int pageSize, int count, vm_acc_t flags, uint64_t *physicalAddressPtr)
{
	// Find a fitting node
	uint64_t len = FRAME_SIZE * count;
	if(pageSize == SIZE_2M)
		len = FRAME_SIZE_2M * count;
	else if(pageSize == SIZE_1G)
		len = FRAME_SIZE_1G * count;
	heap_node_t *node = find_node(len, FRAME_SIZE_1G); // 1G alignment
	if(!node)
		return 0;
	
	// Try to allocate and map memory
	if(!range_alloc_contiguous(node->start, pageSize, count, flags, physicalAddressPtr))
	{
		_heap_free((void *)node->start);
		return 0;
	}
	
	// Node is allocated
	node->state = HEAP_ALLOCATED;
	node->flags = flags;
	return (void *)node->start;
}

// Allocates memory of the given size on the heap.
// Parameters:
// - size: The size of the memory to be allocated (is rounded up to page alignment).
// - flags: Access flags.
// - phy_alloc: Determines whether physical memory shall be allocated.
// - contiguous: Determines whether the allocated physical memory shall be contiguous.
// - physicalAddressPtr: A pointer to a variable where the physical address of the allocated *contiguous* memory is written to.
static void *_heap_alloc(size_t size, vm_acc_t flags, bool phy_alloc, bool contiguous, uint64_t *physicalAddressPtr)
{
    /* round up the size such that it is a multiple of the page size */
    size = PAGE_ALIGN(size);
	
	// Allocate physical memory, or just reserve?
    if(phy_alloc)
    {
        // Contiguous or standard allocation?
		if(contiguous)
		{
			// Calculate needed amount and possibly wasted space for 1G pages
			int size4kAmount = size / FRAME_SIZE;
			int size2mAmount = (size + FRAME_SIZE_2M - 1) / FRAME_SIZE_2M;
			int size1gAmount = (size + FRAME_SIZE_1G - 1) / FRAME_SIZE_1G;
			int size1gWasted = size1gAmount * FRAME_SIZE_1G - size;
			
			// Very small amount of 4K pages?
			void *addr;
			bool tried4k = false;
			if(size4kAmount <= 8)
			{
				// First try 4K allocation, before wasting a larger page
				addr = _heap_alloc_contiguous(SIZE_4K, size4kAmount, flags, physicalAddressPtr);
				if(addr)
					return addr;
				tried4k = true;
			}
			
			// 1G page allocation acceptable?
			bool tried1g = false;
			if(enable1gPages && size1gWasted < TOLERABLE_PAGE_SPACE_WASTE_1G)
			{
				// Try 1G
				addr = _heap_alloc_contiguous(SIZE_1G, size1gAmount, flags, physicalAddressPtr);
				if(addr)
					return addr;
				tried1g = true;
			}

			// Try 2M
			addr = _heap_alloc_contiguous(SIZE_2M, size2mAmount, flags, physicalAddressPtr);
			if(addr)
				return addr;

			// Try 4K
			if(!tried4k)
			{
				addr = _heap_alloc_contiguous(SIZE_4K, size4kAmount, flags, physicalAddressPtr);
				if(addr)
					return addr;
			}

			// Try 1G again, even if it is bad
			if(enable1gPages && !tried1g)
			{
				addr = _heap_alloc_contiguous(SIZE_1G, size1gAmount, flags, physicalAddressPtr);
				if(addr)
					return addr;
			}
			
			// Allocation failed
			return 0;
		}
		else
		{

			/* find a node that can satisfy the size */
			heap_node_t *node = find_node(size, FRAME_SIZE);
			if(!node)
				return 0;
			
			/* allocate physical frames and map them into memory */
			if(!range_alloc(node->start, size, flags))
			{
				_heap_free((void *)node->start);
				return 0;
			}
			
			/* change the state to allocated so heap_free releases the frames */
			node->state = HEAP_ALLOCATED;
			node->flags = flags;
			return (void *)((uintptr_t) node + FRAME_SIZE);
		}
    }
	else
	{
		/* find a node that can satisfy the size */
		heap_node_t *node = find_node(size, FRAME_SIZE);
		if(!node)
			return 0;
		return (void *)((uintptr_t)node + FRAME_SIZE);
	}
}

void *heap_reserve(size_t size)
{
    spin_lock(&heap_lock);
    void *ptr = _heap_alloc(size, 0, false, false, 0);
    spin_unlock(&heap_lock);
    return ptr;
}

void *heap_alloc(size_t size, vm_acc_t flags)
{
    spin_lock(&heap_lock);
    void *ptr = _heap_alloc(size, flags, true, false, 0);
    spin_unlock(&heap_lock);
    return ptr;
}

void *heap_alloc_contiguous(size_t size, vm_acc_t flags, uint64_t *physicalAddressPtr)
{
	spin_lock(&heap_lock);
    void *ptr = _heap_alloc(size, flags, true, true, physicalAddressPtr);
    spin_unlock(&heap_lock);
    return ptr;
}

void heap_free(void *ptr)
{
    spin_lock(&heap_lock);
    _heap_free(ptr);
    spin_unlock(&heap_lock);
}

void heap_trace(void)
{
    spin_lock(&heap_lock);

    trace_printf("Tracing kernel heap...\n");
    for(heap_node_t *node = heap_root; node; node = node->next)
    {
        const char *state = "free";
        const char *r = "", *w = "", *x = "";

        if(node->state == HEAP_RESERVED)
            state = "reserved";
        else if(node->state == HEAP_ALLOCATED)
            state = "allocated ";

        if(node->state == HEAP_ALLOCATED)
        {
            r = node->flags & VM_R ? "r" : "-";
            w = node->flags & VM_W ? "w" : "-";
            x = node->flags & VM_X ? "x" : "-";
        }

        trace_printf(" => %0#18x -> %0#18x (%s%s%s%s)\n", node->start, node->end, state, r, w, x);
    }

    spin_unlock(&heap_lock);
}
