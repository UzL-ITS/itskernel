
#include <acpi/mcfg.h>
#include <trace/trace.h>
#include <mm/mmio.h>
#include <panic/panic.h>
#include <pci/pci.h>


void mcfg_scan(mcfg_t *mcfg)
{
	trace_printf("Scanning MCFG entries...\n");
	
	// Calculate amount of entries
	uint32_t entryArraySize = mcfg->header.len - sizeof(mcfg->header) - sizeof(mcfg->reserved);
	if(entryArraySize % 16)
		panic("MCFG entry array size is not a multiple of 16");
	int entryCount = entryArraySize / 16;
	
	// Run through MCFG entries
	mcfg_entry_t *entry = &mcfg->entries[0];
	for(int i = 0; i < entryCount; ++i)
	{
		trace_printf("Entry #%d:\t %#016x\t %#04x\t %d\t %d\t\n", i, entry->baseAddress, entry->pciSegmentGroupNumber, entry->startPciBusNumber, entry->endPciBusNumber);
		
		// Do a brute force search along all bus numbers, devices and functions
		uint64_t c = entry->endPciBusNumber - entry->startPciBusNumber;
		for(uint64_t b = 0; b < c; ++b)
			for(uint64_t d = 0; d < 32; ++d)
				for(uint64_t f = 0; f < 8; ++f)
				{
					// Calculate and map physical address of configuration space
					uint64_t physicalAddress = entry->baseAddress | (b << 20) | (d << 15) | (f << 12);
					void *cfgSpace = mmio_map(physicalAddress, 4096, VM_R);
					if(!cfgSpace)
						panic("Couldn't map PCIe device configuration space");
					
					// Valid entry?
					uint32_t devVendor = *((uint32_t *)cfgSpace);
					if(devVendor != 0 && devVendor != 0xFFFFFFFF)
					{
						// Save entry in PCI device list
						pci_cfgspace_header_common_t *cfgSpaceHeaderCommon = (pci_cfgspace_header_common_t *)cfgSpace;
						pci_add_discovered_device(cfgSpaceHeaderCommon);
						
						// Debug output
						trace_printf("     [%02x,%02x,%01x] Vendor %04x  Device %04x  Command %04x  Status %04x  Class %02x:%02x  Header %02x\n",
						    b, d, f,
							cfgSpaceHeaderCommon->vendorId,
							cfgSpaceHeaderCommon->deviceId,
							cfgSpaceHeaderCommon->command,
							cfgSpaceHeaderCommon->status,
							cfgSpaceHeaderCommon->classCode,
							cfgSpaceHeaderCommon->subclassCode,
							cfgSpaceHeaderCommon->headerType
						);
						if(cfgSpaceHeaderCommon->headerType == 0x00)
						{
							pci_cfgspace_header_0_t *cfgSpaceHeader0 = (pci_cfgspace_header_0_t *)cfgSpaceHeaderCommon;
							trace_printf("        PIC-IRQ %02x  Int-Pin %02x (INT%c#)\n",
								cfgSpaceHeader0->interruptLine,
								cfgSpaceHeader0->interruptPin,
								'@' + cfgSpaceHeader0->interruptPin
							);
						}
						
						// Keep this configuration space mapped
					}
					else
					{
						// Unmap this configuration space
						mmio_unmap(cfgSpace, 4096);
					}
				}
		
		// Next MCFG entry
		++entry;
	}
	trace_printf("MCFG scan successful.\n");
}
