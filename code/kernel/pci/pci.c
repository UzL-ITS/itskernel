/*
PCI driver.
*/

#include <pci/pci.h>
#include <trace/trace.h>
#include <cpu/port.h>
#include <util/list.h>
#include <util/container.h>
#include <stdlib/stdlib.h>
#include <panic/panic.h>
#include <intr/route.h>
#include <cpu/state.h>
#include <net/net.h>

// Represents one discovered PCI device.
typedef struct pci_device
{
	// A pointer to the memory mapped extended configuration space.
	pci_cfgspace_header_common_t *deviceCfgSpaceHeaderCommon;
	
	// Pointer to the next device in the device list.
	struct pci_device *next;
} pci_device_t;

// The memory mapped PCI devices.
pci_device_t *discoveredDevicesList = 0;

void pci_add_discovered_device(pci_cfgspace_header_common_t *deviceCfgSpaceHeaderCommon)
{	
	// Allocate device list node
	pci_device_t *deviceNode = (pci_device_t *)malloc(sizeof(pci_device_t));
	deviceNode->deviceCfgSpaceHeaderCommon = deviceCfgSpaceHeaderCommon;
	
	// Add device to list
	deviceNode->next = discoveredDevicesList;
	discoveredDevicesList = deviceNode;
}

// Returns the index of the lowest set bit for the given value != 0.
static uint32_t get_number_of_lowest_set_bit(uint32_t value)
{
	if(value == 0)
		return 0xFFFFFFFF;
	uint32_t pos = 0;
	uint32_t mask = 0x00000001;
	while((value & mask) == 0x00000000)
	{
		mask <<= 1;
		++pos;
	}
	return pos;
}

// Handles PCI interrupts.
static void _handle_pci_interrupt(cpu_state_t *state)
{
	//trace_printf("Received PCI interrupt.\n");
	
	// Pass interrupt to responsible device
	net_handle_interrupt(state);
	//trace_printf("PCI interrupt handle end.\n");
}

// Prints the BAR configuration of the given device.
static void print_bar_info(pci_cfgspace_header_0_t *deviceCfgSpaceHeader)
{
	for(int i = 0; i < 6; ++i)
	{
		trace_printf("BAR #%d: %08x\n", i, deviceCfgSpaceHeader->bars.bars32[i]);
		
		pci_bar_info_t info;
		pci_get_bar_info(deviceCfgSpaceHeader, i, &info);
		
		if(info.isMmio)
		{
			trace_printf("    memory at %016x, %s, %d bits, %016x bytes\n",
				info.baseAddress,
				info.isPrefetchable ? "prefetchable" : "non-prefetchable",
				info.bitness,
				info.size
			);
		}
		else
		{
			trace_printf("    I/O resource at %016x, %016x bytes\n",
				info.baseAddress,
				info.size
			);
		}
	}
}

void pci_get_bar_info(pci_cfgspace_header_0_t *deviceCfgSpaceHeader, int barIndex, pci_bar_info_t *infoPtr)
{
	// Check BAR type
	uint32_t *barPtr = &deviceCfgSpaceHeader->bars.bars32[barIndex];
	if(!(*barPtr & 1))
	{
		// Retrieve base address, bitness and size
		uint64_t baseAddress = 0;
		uint8_t bitness = 32;
		uint8_t barSizeLog = 0;
		switch((*barPtr >> 1) & 3)
		{
			case 0:
			{
				bitness = 32;
				baseAddress = *barPtr & 0xFFFFFFF0;
				
				uint32_t oldBarValue = *barPtr;
	
				*barPtr = 0xFFFFFFF0;
				uint32_t newBarValue = *barPtr & 0xFFFFFFF0;
				if(newBarValue != 0)
					barSizeLog = get_number_of_lowest_set_bit(newBarValue);
				
				*barPtr = oldBarValue;
		
				break;
			}
			case 2:
			{
				bitness = 64;
				baseAddress = (*((uint64_t *)(barPtr + 1)) << 32) + (*barPtr & 0xFFFFFFF0);
					
				uint32_t oldBarValue = *barPtr;
				uint32_t oldBarValue2 = *(barPtr + 1);
				
				*barPtr = 0xFFFFFFF0;
				*(barPtr + 1) = 0xFFFFFFFF;
				uint32_t newBarValueLow = *barPtr & 0xFFFFFFF0;
				uint32_t newBarValueHigh = *(barPtr + 1);
				
				if(newBarValueLow != 0)
					barSizeLog = get_number_of_lowest_set_bit(newBarValueLow);
				else
					barSizeLog = get_number_of_lowest_set_bit(newBarValueHigh) + 32;
				
				*barPtr = oldBarValue;
				*(barPtr + 1) = oldBarValue2;
				
				break;
			}
			
			default:
				panic("Undefined BAR bitness!");
				break;
		}
		
		// Save values
		infoPtr->baseAddress = baseAddress;
		infoPtr->bitness = bitness;
		infoPtr->isMmio = true;
		infoPtr->isPrefetchable = (*barPtr & 0x03) ? true : false;
		infoPtr->size = ((uint64_t)1 << barSizeLog);
	}
	else
	{
		uint64_t baseAddress = *barPtr & 0xFFFFFFF0;
		uint32_t oldBarValue = *barPtr;
				
		*barPtr = 0xFFFFFFFC;
		uint32_t newBarValue = *barPtr & 0xFFFFFFFC;
		
		uint8_t barSizeLog = get_number_of_lowest_set_bit(newBarValue);
		
		*barPtr = oldBarValue;
		
		// Save values
		infoPtr->baseAddress = baseAddress;
		infoPtr->bitness = 0;
		infoPtr->isMmio = false;
		infoPtr->isPrefetchable = false;
		infoPtr->size = ((uint64_t)1 << barSizeLog);
	}
}

void pci_init()
{
	trace_printf("PCI init...\n");

	// Route PCI interrupts INTA ... INTD
	// TODO Detect IRQs with ACPI AML parsing
	for(int i = 0; i < 4; ++i)
	{
		irq_tuple_t tuple;
		tuple.irq = 16 + i;
		tuple.active_polarity = POLARITY_LOW;
		tuple.trigger = TRIGGER_LEVEL;
		if(!intr_route_irq(&tuple, _handle_pci_interrupt))
			trace_printf("Error: Could not install PCI interrupt INT%c handler.\n", (char)('A' + i));
		else
			trace_printf("PCI interrupt INT%c handler successfully installed.\n", (char)('A' + i));
	}
	
	// Find ethernet card
	for(pci_device_t *cur = discoveredDevicesList; cur != 0; cur = cur->next)
	{
		// Check type
		if(cur->deviceCfgSpaceHeaderCommon->classCode == 0x02 && cur->deviceCfgSpaceHeaderCommon->subclassCode == 0x00)
		{
			// Debug
			trace_printf("Ethernet controller found: VendorID %04x, DeviceID %04x\n", cur->deviceCfgSpaceHeaderCommon->vendorId, cur->deviceCfgSpaceHeaderCommon->deviceId);
			
			pci_cfgspace_header_0_t *deviceCfgSpaceHeader = (pci_cfgspace_header_0_t *)cur->deviceCfgSpaceHeaderCommon;
			
			print_bar_info(deviceCfgSpaceHeader);
			net_init(deviceCfgSpaceHeader);
		}
	}
}