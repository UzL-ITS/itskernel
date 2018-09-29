
#include <proc/syscalls.h>
#include <smp/cpu.h>
#include <smp/topology.h>

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
				buffer[3 * p + 0] = t->packageId;
				buffer[3 * p + 1] = t->coreId;
				buffer[3 * p + 2] = t->smtId;
			}
		}
	}
}