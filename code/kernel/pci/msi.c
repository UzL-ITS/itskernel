/*
Message Signaled Interrupts (MSI) implementation.
*/
#include <pci/msi.h>
#include <panic/panic.h>
#include <trace/trace.h>
#include <smp/cpu.h>


// The MSI base address.
// TODO read APIC base address
static uint64_t msiBaseAddress = 0xFEE00000;


void msi_enable(pci_cap_common_t *pciCapHeader, intr_t intr, intr_handler_t handler)
{
	// Get capability object header
	pci_cap_msi_header_t *pciMsiCapHeader = (pci_cap_msi_header_t *)pciCapHeader;
	
	// We always route to CPU #0
	cpu_t *cpu = cpu_get_bsp();
	
	// MSI address
	uint8_t destId = cpu->lapic_id;
	uint8_t addressFlags = 0x08;
	uint32_t address = msiBaseAddress | (destId << 12) | addressFlags;
	
	// MSI data
	uint16_t data = 0x00000000 | intr; // All flags zero, i.e. edge triggered, fixed delivery mode
	
	trace_printf("MSI address: %08x\n", address);
	trace_printf("MSI data: %04x\n", data);
	
	// Assign address and data, depending on 32/64 bit mode
	if(pciMsiCapHeader->messageControl & PCI_MSI_MESSAGE_CTRL_64BIT_SUPPORTED)
	{
		pci_cap_msi64_t *pciMsiCap = (pci_cap_msi64_t *)pciMsiCapHeader;
		pciMsiCap->messageAddressLow = address;
		pciMsiCap->messageAddressHigh = 0x00000000;
		pciMsiCap->messageData = data;
		
		if(pciMsiCapHeader->messageControl & PCI_MSI_MESSAGE_CTRL_VECTOR_SUPPORTED)
			trace_printf("64-bit MSI, Mask/Pending: %08x  %08x\n", pciMsiCap->maskBits, pciMsiCap->pendingBits);
	}
	else
	{
		pci_cap_msi32_t *pciMsiCap = (pci_cap_msi32_t *)pciMsiCapHeader;
		pciMsiCap->messageAddress = address;
		pciMsiCap->messageData = data;
		
		if(pciMsiCapHeader->messageControl & PCI_MSI_MESSAGE_CTRL_VECTOR_SUPPORTED)
			trace_printf("32-bit MSI, Mask/Pending: %08x  %08x\n", pciMsiCap->maskBits, pciMsiCap->pendingBits);
	}
	
	// Install interrupt handler
	intr_route_intr(intr, handler);
	
	// TODO multiple message enable
	
	// Enable MSI
	pciMsiCapHeader->messageControl |= PCI_MSI_MESSAGE_CTRL_ENABLE;
	trace_printf("MSI control register: %04x\n", pciMsiCapHeader->messageControl);
}