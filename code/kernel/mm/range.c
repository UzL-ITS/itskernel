
#include <mm/range.h>
#include <mm/common.h>
#include <mm/pmm.h>
#include <mm/vmm.h>
#include <stdlib/assert.h>
#include <panic/panic.h>
#include <trace/trace.h>

bool range_alloc(uintptr_t addr_start, size_t len, vm_acc_t flags)
{
	assert((len % FRAME_SIZE) == 0);

	for(uintptr_t addr = addr_start, addr_end = addr + len; addr < addr_end;)
	{
		size_t remaining = addr_end - addr;

		/* try to use a 1G frame */
		if(enable1gPages && (addr % FRAME_SIZE_1G) == 0 && remaining >= FRAME_SIZE_1G)
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
		if(enable2mPages && (addr % FRAME_SIZE_2M) == 0 && remaining >= FRAME_SIZE_2M)
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

bool range_alloc_contiguous(uintptr_t virtualAddress, int size, int count, vm_acc_t flags, uint64_t *physicalAddress)
{
	// Get frame size
	uint64_t frameSize = FRAME_SIZE;
	if(size == SIZE_2M)
		frameSize = FRAME_SIZE_2M;
	else if(size == SIZE_1G)
		frameSize = FRAME_SIZE_1G;
	
	// Ask PMM to allocate a sufficient amount of contiguous memory
	uintptr_t frames = pmm_alloc_contiguous(size, count);
	if(frames)
	{
		// Try to map virtual memory
		bool success = true;
		if(!vmm_map_range(virtualAddress, frames, count * frameSize, flags))
			success = false;
		/*uintptr_t currVirtAddr = virtualAddress;
		for(int i = 0; i < count; ++i)
		{
			// Map
			if(vmm_maps(currVirtAddr, frames + i * frameSize, flags, size))
			{
				// Next virtual address block
				currVirtAddr += frameSize;
			}
			else
			{
				// Revert all already successed mappings
				--i;
				currVirtAddr -= frameSize;
				for(; i >= 0; --i)
				{
					vmm_unmap(currVirtAddr);
					currVirtAddr -= frameSize;
				}
				success = false;
				break;
			}
		}*/
		
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
			for(int i = 0; i < count; ++i)
				pmm_frees(size, frames + i * frameSize);
		}
	}
	return false;
}

void range_free(uintptr_t addr, size_t len)
{
	assert((len % FRAME_SIZE) == 0);

	for(uintptr_t addr_end = addr + len; addr < addr_end;)
	{
		int size = vmm_size(addr);
		if(size == -1)
			panic("pmm range_free: vmm_size returned -1");
		
		uint64_t phyAddr = vmm_unmaps(addr, size);
		switch(size)
		{
			case SIZE_1G:
				pmm_frees(SIZE_1G, phyAddr);
				addr += FRAME_SIZE_1G;
				break;

			case SIZE_2M:
				pmm_frees(SIZE_2M, phyAddr);
				addr += FRAME_SIZE_2M;
				break;
			
			default:
				pmm_free(phyAddr);
				addr += FRAME_SIZE;
				break;
		}
	}
}
