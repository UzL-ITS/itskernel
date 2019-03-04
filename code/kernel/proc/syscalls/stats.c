
#include <proc/syscalls.h>
#include <smp/cpu.h>
#include <smp/topology.h>
#include <mm/pmm.h>
#include <cpu/cpuid.h>

uint64_t sys_get_elapsed_milliseconds()
{
	// Get counter of bootstrap CPU
	cpu_t *cpu = cpu_get_bsp();
	return cpu->elapsedMsSinceStart;
}

void sys_info(int infoId, uint8_t *buffer)
{
	// Act depending on information ID
	uint32_t *buffer32 = (uint32_t *)buffer;
	uint64_t *buffer64 = (uint64_t *)buffer;
	switch(infoId)
	{
		// Return CPU count
		// Buffer size: 4 Bytes
		case 0:
		{
			buffer32[0] = topology_get_count();
			break;
		}
		
		// Return CPU topology
		// Buffer size: processorCount * 12 Bytes
		case 1:
		{
			// Run through processors and copy topology information
			int processorCount = topology_get_count();
			for(int p = 0; p < processorCount; ++p)
			{
				// Copy topology data
				const processor_topology_t *t = topology_get(p);
				buffer32[3 * p + 0] = t->packageId;
				buffer32[3 * p + 1] = t->coreId;
				buffer32[3 * p + 2] = t->smtId;
			}
			break;
		}
		
		// Return amount of available physical memory
		// Buffer size: 8 Bytes
		case 2:
		{
			buffer64[0] = pmm_get_available_memory();
			break;
		}
		
		// Return information about AMD memory encryption
		// Buffer size: 8 Bytes
		case 3:
		{
			// Get info
			uint32_t eax;
			uint32_t ebx;
			uint32_t tmp;
			cpu_id(0x8000001F, &eax, &ebx, &tmp, &tmp);
			buffer32[0] = eax;
			buffer32[1] = ebx;
		}
	}
}

void sys_dump(int infoId, const char *filePath)
{
	// Act depending on information ID
	switch(infoId)
	{
		// Dump PMM state
		case 0:
		{
			pmm_dump_stack(filePath);
			break;
		}
	}
}