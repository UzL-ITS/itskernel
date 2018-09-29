/*
Processor topology storage.

Uses algorithms from
    https://software.intel.com/en-us/articles/intel-64-architecture-processor-topology-enumeration
*/

#include <smp/topology.h>
#include <cpu/cpuid.h>
#include <stdlib/stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <panic/panic.h>

// Tests whether the given bit it set.
#define BIT_IS_SET(var, bit) ((var) & (0x1 << bit))

// The amount of CPUs.
static int processorCount;

// CPU topology list (ordered by internal core ID).
static processor_topology_t **topologyData = 0;

// Masks for deriving topology information from a processor's APIC ID.
static uint32_t coreSelectMask;
static uint32_t smtMaskWidth;
static uint32_t packageSelectMask;
static uint32_t packageSelectMaskShift;
static uint32_t smtSelectMask;

// The highest supported CPUID leaf (EAX parameter).
uint32_t highestSupportedCpuIdLeaf;

// Determines whether the CPU supports CPUID leaf 0x0B.
static bool leaf0BSupported;

void topology_prepare(int pCount)
{
	// Allocate topology array
	processorCount = pCount;
	topologyData = (processor_topology_t **)malloc(processorCount * sizeof(processor_topology_t *));
	
	// Dummy variable for unused CPUID values
	uint32_t tmp;
	
	// Check whether hyperthreading is supported
	uint32_t leaf_01_00_EDX;
	cpu_id_special(0x01, 0x00, &tmp, &tmp, &tmp, &leaf_01_00_EDX);
	if(BIT_IS_SET(leaf_01_00_EDX, 28))
	{	
		// Get highest supported CPUID function leaf
		cpu_id_special(0x00, 0x00, &highestSupportedCpuIdLeaf, &tmp, &tmp, &tmp);
		
		// Check whether leaf 0x0B is really supported
		uint32_t leaf_0B_00_EAX;
		uint32_t leaf_0B_00_EBX;
		uint32_t leaf_0B_00_ECX;
		uint32_t leaf_0B_00_EDX;
		leaf0BSupported = false;
		if(highestSupportedCpuIdLeaf >= 0x0B)
		{
			cpu_id_special(0x0B, 0x00, &leaf_0B_00_EAX, &leaf_0B_00_EBX, &leaf_0B_00_ECX, &leaf_0B_00_EDX);
			if(leaf_0B_00_EBX)
				leaf0BSupported = true;
		}
		
		// Choose way to derive parameters
		if(leaf0BSupported)
		{
			// Retrieve data from leaf 0x0B
			uint32_t subLeaf = 0;
			bool coreEncountered = false;
			bool smtEncountered = false;
			uint32_t coreAndSmtMask = 0x00000000;
			uint32_t lastLevelShift = 0;
			while(true)
			{
				// Check type of current level
				switch((leaf_0B_00_ECX >> 8) & 0xFF)
				{
					case 1:
					{
						// SMT (logical core)
						smtEncountered = true;
						
						// Calculate SMT mask
						lastLevelShift = leaf_0B_00_EAX & 0x1F;
						smtMaskWidth = lastLevelShift;
						smtSelectMask = ~(0xFFFFFFFF << smtMaskWidth);
						break;
					}
					
					case 2:
					{
						// Physical core
						coreEncountered = true;
						
						// Calculate core mask
						lastLevelShift = leaf_0B_00_EAX & 0x1F;
						coreAndSmtMask = ~(0xFFFFFFFF << lastLevelShift);
						packageSelectMaskShift = lastLevelShift;
						packageSelectMask = 0xFFFFFFFF ^ coreAndSmtMask;
						break;
					}
					
					default:
					{
						break;
					}
				}
				
				// Next sub leaf
				++subLeaf;
				cpu_id_special(0x0B, subLeaf, &leaf_0B_00_EAX, &leaf_0B_00_EBX, &leaf_0B_00_ECX, &leaf_0B_00_EDX);
				if(!leaf_0B_00_EBX)
					break;
			}
			
			// Calculate core mask
			// Since hyperthreading is enabled, there should be information about a logical and a physical core
			if(coreEncountered && smtEncountered)
				coreSelectMask = coreAndSmtMask ^ smtSelectMask;
			else if(!coreEncountered && smtEncountered)
			{
				// Strange, but we handle this
				coreSelectMask = 0x00000000;
				packageSelectMaskShift = smtMaskWidth;
				packageSelectMask = 0xFFFFFFFF ^ smtSelectMask;
			}
			else
				panic("Hyperthreading is enabled, but CPUID gives contradicting information about topology");
		}
		else
		{
			// TODO
			panic("TODO Implement CPUID 0x04 topology mask detection");
		}
	}
	else
	{
		// No hyperthreading support, thus there is one logical core per physical core
		coreSelectMask = 0x00000000;
		smtMaskWidth = 0;
		packageSelectMask = 0xFFFFFFFF;
		packageSelectMaskShift = 0;
		smtSelectMask = 0x00000000;
	}
}

void topology_init(cpu_t *cpu)
{
	// Allocate topology data structure for the given processor
	processor_topology_t *topo = (processor_topology_t *)malloc(sizeof(processor_topology_t));
	topologyData[cpu->coreId] = topo;
	
	// Dummy variable for unused CPUID values
	uint32_t tmp;
	
	// Retrieve processor's APIC ID
	uint32_t apicId;
	if(leaf0BSupported)
		cpu_id_special(0x0B, 0x00, &tmp, &tmp, &tmp, &apicId);
	else
	{
		cpu_id_special(0x01, 0x00, &tmp, &apicId, &tmp, &tmp);
		apicId >>= 24; // Highest 8 Bits of EBX
	}
	
	// Split APIC ID and fill topology entry
	topo->packageId = ((apicId & packageSelectMask) >> packageSelectMaskShift);
	topo->coreId = ((apicId & coreSelectMask) >> smtMaskWidth);
	topo->smtId = (apicId & smtSelectMask);
}

int topology_get_count()
{
	return processorCount;
}

const processor_topology_t *topology_get(int coreId)
{
	// Sanity check
	if(coreId < 0 || coreId >= processorCount)
		return 0;
	return topologyData[coreId];
}