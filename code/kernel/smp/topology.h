#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <smp/cpu.h>

// Contains topology data of one processor.
typedef struct
{
	// The ID of the hardware chip (package).
	uint32_t packageId;
	
	// The ID of the physical core on the given chip.
	uint32_t coreId;
	
	// The ID of the logical (SMT) core within the given physical core.
	uint32_t smtId;
} processor_topology_t;

// Prepares the internal topology array.
void topology_prepare(int processorCount);

// Saves topology data for the current processor.
void topology_init(cpu_t *cpu);

// Retrieves the processor count.
int topology_get_count();

// Retrieves topology data for the processor with the given ID.
const processor_topology_t *topology_get(int coreId);