
#include <proc/syscalls.h>
#include <mm/seg.h>
#include <mm/common.h>
#include <mm/vmm.h>

void *sys_heap_alloc(int size)
{
	// Make sure size is multiple of 4K
	if(size % FRAME_SIZE != 0)
		size += FRAME_SIZE - (size % FRAME_SIZE);
	return seg_alloc(size, VM_R | VM_W);
}

void sys_heap_free(void *addr)
{
	seg_free(addr);
}

uint64_t sys_virt_to_phy(uint64_t addr)
{
	return vmm_virt_to_phys(addr);
}

void sys_hugepage_mode(bool enable)
{
	enable2mPages = enable;
}

uint64_t sys_page_flags(uint64_t address, uint64_t flags, bool set)
{
	return vmm_modify_flags(address, flags, set);
}