
#include <mm/range.h>
#include <mm/common.h>
#include <mm/pmm.h>
#include <mm/vmm.h>
#include <stdlib/assert.h>
#include <panic/panic.h>

// The tolerable amount of unused space on an allocated large page.
// Used for determining a good fit for contiguous allocations.
#define TOLERABLE_PAGE_SPACE_WASTE_1G 536870912 // 50%

bool range_alloc(uintptr_t addr_start, size_t len, vm_acc_t flags)
{
	assert((len % FRAME_SIZE) == 0);

	for(uintptr_t addr = addr_start, addr_end = addr + len; addr < addr_end;)
	{
		size_t remaining = addr_end - addr;

		/* try to use a 1G frame */
		if((addr % FRAME_SIZE_1G) == 0 && remaining >= FRAME_SIZE_1G)
		{
			uintptr_t frame = pmm_allocs(SIZE_1G);
			if(frame)
			{
				if(vmm_maps(addr, frame, flags, SIZE_1G))
				{
					addr += FRAME_SIZE_1G;
					continue;
				}
				else
				{
					pmm_frees(frame, SIZE_1G);
				}
			}
		}

		/* try to use a 2M frame */
		if((addr % FRAME_SIZE_2M) == 0 && remaining >= FRAME_SIZE_2M)
		{
			uintptr_t frame = pmm_allocs(SIZE_2M);
			if(frame)
			{
				if(vmm_maps(addr, frame, flags, SIZE_2M))
				{
					addr += FRAME_SIZE_2M;
					continue;
				}
				else
				{
					pmm_frees(frame, SIZE_2M);
				}
			}
		}

		/* try to use a 4K frame */
		uintptr_t frame = pmm_alloc();
		if(!frame)
		{
			range_free(addr_start, len);
			return false;
		}

		if(!vmm_map(addr, frame, flags))
		{
			pmm_free(frame);
			range_free(addr_start, len);
			return false;
		}

		addr += FRAME_SIZE;
	}

	return true;
}

bool range_alloc_contiguous(uintptr_t addr_start, size_t len, vm_acc_t flags, uint64_t *physicalAddress)
{
	// TODO pmm_alloc_contiguous fehlt noch
	
	// Check length, should be aligned to at least 4k
	assert((len % FRAME_SIZE) == 0);

	// Calculate needed amount and possibly wasted space for 1G pages
	int size4kAmount = len / FRAME_SIZE;
	int size2mAlignment = addr_start % FRAME_SIZE_2M; // The wasted space for 2M alignment
	int size2mAmount = (len + size2mAlignment + FRAME_SIZE_2M - 1) / FRAME_SIZE_2M;
	int size1gAlignment = addr_start % FRAME_SIZE_1G; // The wasted space for 1G alignment
	int size1gAmount = (len + size1gAlignment + FRAME_SIZE_1G - 1) / FRAME_SIZE_1G;
	int size1gWasted = size1gAmount * FRAME_SIZE_1G - len;
	
	// 1G page allocation acceptable?
	if(size1gWasted < TOLERABLE_PAGE_SPACE_WASTE_1G)
	{
		// Ask PMM to allocate a sufficient amount of contiguous memory
		uintptr_t frames = pmm_alloc_contiguous(SIZE_1G, size1gAmount);
		if(frames)
		{
			// Try to map virtual memory
			bool success = true;
			uintptr_t currVirtAddr = addr_start;
			for(int i = 0; i < size1gAmount; ++i)
			{
				// Map
				if(vmm_maps(currVirtAddr, frames + i * FRAME_SIZE_1G, flags, SIZE_1G))
				{
					// Next virtual address block
					currVirtAddr += FRAME_SIZE_1G;
				}
				else
				{
					// Revert all already successed mappings
					--i;
					currVirtAddr -= FRAME_SIZE_1G;
					for(; i >= 0; --i)
					{
						vmm_unmap(currVirtAddr);
						currVirtAddr -= FRAME_SIZE_1G;
					}
					success = false;
					break;
				}
			}
			
			// Success?
			if(success)
			{
				if(physicalAddress)
					*physicalAddress = frames;
				return true;
			}
			else
			{
				// Release physical pages again
				for(int i = 0; i < size1gAmount; ++i)
					pmm_frees(frames + i * FRAME_SIZE_1G, SIZE_1G);
			}
		}
		
		// Failed, continue
	}
	
	// 2M
	{
		// Ask PMM to allocate a sufficient amount of contiguous memory
		uintptr_t frames = pmm_alloc_contiguous(SIZE_2M, size2mAmount);
		if(frames)
		{
			// Try to map virtual memory
			bool success = true;
			uintptr_t currVirtAddr = addr_start;
			for(int i = 0; i < size2mAmount; ++i)
			{
				// Map
				if(vmm_maps(currVirtAddr, frames + i * FRAME_SIZE_2M, flags, SIZE_2M))
				{
					// Next virtual address block
					currVirtAddr += FRAME_SIZE_2M;
				}
				else
				{
					// Revert all already successed mappings
					--i;
					currVirtAddr -= FRAME_SIZE_2M;
					for(; i >= 0; --i)
					{
						vmm_unmap(currVirtAddr);
						currVirtAddr -= FRAME_SIZE_2M;
					}
					success = false;
					break;
				}
			}
			
			// Success?
			if(success)
			{
				if(physicalAddress)
					*physicalAddress = frames;
				return true;
			}
			else
			{
				// Release physical pages again
				for(int i = 0; i < size2mAmount; ++i)
					pmm_frees(frames + i * FRAME_SIZE_2M, SIZE_2M);
			}
		}
		
		// Failed, continue
	}
	
	// 4K
	{
		// Ask PMM to allocate a sufficient amount of contiguous memory
		uintptr_t frames = pmm_alloc_contiguous(SIZE_4K, size4kAmount);
		if(frames)
		{
			// Try to map virtual memory
			bool success = true;
			uintptr_t currVirtAddr = addr_start;
			for(int i = 0; i < size4kAmount; ++i)
			{
				// Map
				if(vmm_maps(currVirtAddr, frames + i * FRAME_SIZE, flags, SIZE_4K))
				{
					// Next virtual address block
					currVirtAddr += FRAME_SIZE;
				}
				else
				{
					// Revert all already successed mappings
					--i;
					currVirtAddr -= FRAME_SIZE;
					for(; i >= 0; --i)
					{
						vmm_unmap(currVirtAddr);
						currVirtAddr -= FRAME_SIZE;
					}
					success = false;
					break;
				}
			}
			
			// Success?
			if(success)
			{
				if(physicalAddress)
					*physicalAddress = frames;
				return true;
			}
			else
			{
				// Release physical pages again
				for(int i = 0; i < size4kAmount; ++i)
					pmm_frees(frames + i * FRAME_SIZE, SIZE_4K);
			}
		}
		
		// Failed
		return false;
	}
}

void range_free(uintptr_t addr, size_t len)
{
	assert((len % FRAME_SIZE) == 0);

	for(uintptr_t addr_end = addr + len; addr < addr_end;)
	{
		int size = vmm_size(addr);
		if(size == -1)
			panic("pmm range_free: vmm_size returned -1");
		
		switch(size)
		{
			case SIZE_1G:
				pmm_frees(FRAME_SIZE_1G, vmm_unmap(addr));
				addr += FRAME_SIZE_1G;
				break;

			case SIZE_2M:
				pmm_frees(FRAME_SIZE_2M, vmm_unmap(addr));
				addr += FRAME_SIZE_2M;
				break;
			
			default:
				pmm_free(vmm_unmap(addr));
				addr += FRAME_SIZE;
				break;
		}
	}
}
