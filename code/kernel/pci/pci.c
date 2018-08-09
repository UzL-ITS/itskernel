
#include <pci/pci.h>
#include <trace/trace.h>
#include <cpu/port.h>
#include <util/list.h>
#include <util/container.h>
#include <stdlib/stdlib.h>
#include <panic/panic.h>

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

/* sucht das unterste gesetzte Bit in einem 32Bit-Wert , value darf nicht 0 sein */
static uint32_t get_number_of_lowest_set_bit(uint32_t value)
{
  uint32_t pos = 0;
  uint32_t mask = 0x00000001;
  while (!(value & mask))
   { ++pos; mask=mask<<1; }
  return pos;
}

/* sucht das oberste gesetzte Bit in einem 32Bit-Wert , value darf nicht 0 sein */
static uint32_t get_number_of_highest_set_bit(uint32_t value)
{
  uint32_t pos = 31;
  uint32_t mask = 0x80000000;
  while (!(value & mask))
   { --pos; mask=mask>>1; }
  return pos;
}

void pci_init()
{
	trace_printf("PCI init...\n");
	
	// Find ethernet card
	for(pci_device_t *cur = discoveredDevicesList; cur != 0; cur = cur->next)
	{
		// Check type
		if(cur->deviceCfgSpaceHeaderCommon->classCode == 0x02 && cur->deviceCfgSpaceHeaderCommon->subclassCode == 0x00)
		{
			// Debug
			trace_printf("Ethernet controller found: VendorID %04x, DeviceID %04x\n", cur->deviceCfgSpaceHeaderCommon->vendorId, cur->deviceCfgSpaceHeaderCommon->deviceId);
			
			pci_cfgspace_header_0_t *deviceCfgSpaceHeader = (pci_cfgspace_header_0_t *)cur->deviceCfgSpaceHeaderCommon;
			
			for(int i = 0; i < 6; ++i)
			{
				trace_printf("BAR #%d: %08x\n", i, deviceCfgSpaceHeader->bars.bars32[i]);
				
				uint32_t *barPtr = &deviceCfgSpaceHeader->bars.bars32[i];
				uint32_t oldBarValue = *barPtr;
				if(!(oldBarValue & 1))
				{
					uint8_t bitness = 32;
					uint8_t barSizeLog = 0;
					switch((oldBarValue >> 1) & 3)
					{
						case 0:
						{
							bitness = 32;
							
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
					
					trace_printf("    memory, %s, %d bits, 2^%u bytes\n",
						(oldBarValue & 0x03) ? "prefetchable" : "non-prefetchable",
						bitness,
						barSizeLog
					);
				}
				else
				{
					*barPtr = 0xFFFFFFFC;
					uint32_t newBarValue = *barPtr & 0xFFFFFFFC;
					
					uint8_t barSizeLog = get_number_of_lowest_set_bit(newBarValue);
					
					*barPtr = oldBarValue;
					
					trace_printf("    I/O resource, 2^%u bytes\n",
						barSizeLog
					);
				}
			}
		}
	}	
}