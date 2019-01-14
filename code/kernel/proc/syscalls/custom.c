
#include <proc/syscalls.h>
#include <vbe/vbe.h>
#include <trace/trace.h>
#include <mm/vmm.h>
#include <stdlib/string.h>


#define PAGES(n) ((n)*4096)
static uint8_t *mem = 0;


uint8_t *sys_custom(int param)
{
	
	int size = PAGES(1024);
	
	uint64_t phys;
	if(mem == 0)
		mem = heap_alloc_contiguous(size, VM_R | VM_W, &phys);
	
	trace_printf("Memory address: virt = %016x    phy = %016x\n", mem, vmm_virt_to_phys((uint64_t)mem));
	
	//int size = 1024 * 768 * 4;
	if(param == 0)
	{
		memset(mem + PAGES(000), 0xFF, PAGES(128));
		memset(mem + PAGES(128), 0x00, PAGES(128));
		memset(mem + PAGES(256), 0xFF, PAGES(128));
		memset(mem + PAGES(384), 0x00, PAGES(128));
		memset(mem + PAGES(512), 0xFF, PAGES(128));
		memset(mem + PAGES(640), 0x00, PAGES(128));
		
	}
	else if(param == 1)
	{
		sys_create_file("/", "mem.bin", mem, size);
	}
	
	trace_printf("sys_test() done.\n");
	
	return 0;
}