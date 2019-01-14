#include <acpi/power.h>
#include <stdint.h>
#include <panic/panic.h>
#include <cpu/port.h>
#include <cpu/halt.h>
#include <trace/trace.h>

// Reset register data.
static fadt_addr_type_t resetAddressType;
static uint64_t resetAddress;
static uint8_t resetValue;


void power_init(fadt_t *fadt)
{
	trace_printf("ACPI power init\n");
	
	// Get reset information
	resetAddressType = fadt->resetReg.addressSpace;
	resetAddress = fadt->resetReg.address;
	resetValue = fadt->resetValue;
	trace_printf("RESET register: Type %02x  Size %02x  Address %016x  Value %02x\n", (uint8_t)resetAddressType, fadt->resetReg.accessSize, resetAddress, resetValue);
}

void power_reset()
{
	// Act depending on reset address type
	switch(resetAddressType)
	{
		case FADT_ADDR_TYPE_SYSTEM_MEMORY:
		{
			panic("System memory based ACPI reset not implemented");
			break;
		}
		
		case FADT_ADDR_TYPE_SYSTEM_IO:
		{
			// Write port
			outb(resetAddress, resetValue);
			break;
		}
		
		case FADT_ADDR_TYPE_PCI_CFG_SPACE:
		{
			panic("PCI config space based ACPI reset not implemented");
			break;
		}
	}
	panic("ACPI reset ...");
}