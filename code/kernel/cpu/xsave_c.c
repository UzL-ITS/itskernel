
#include <cpu/xsave.h>
#include <cpu/cpuid.h>
#include <stdlib/stdlib.h>
#include <trace/trace.h>

void* xsave_alloc(void)
{
	// Determine size of XSAVE region
	uint32_t eax;
	uint32_t ebx;
	uint32_t ecx;
	uint32_t edx;
	cpu_id_special(0x0D, 0x00, &eax, &ebx, &ecx, &edx);
	uint32_t size = ebx;
	
	// Allocate XSAVE region
	void *mem = memalign(64, size);
	
	// Write dummy data to get a valid state
	xsave(mem);
	
	// Done
	return mem;
}

void xsave_free(void *mem)
{
	// Free
	free(mem);
}