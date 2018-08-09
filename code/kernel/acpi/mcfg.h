
#ifndef _ACPI_MCFG_H
#define _ACPI_MCFG_H

#include <stdint.h>
#include <acpi/common.h>

#define MCFG_SIGNATURE 0x4746434D /* 'MCFG' */

// Structure of one configuration space entry.
typedef struct
{
	// Physical base address of enhanced configuration mechanism data.
	uint64_t baseAddress;
	
	// PCI segment group number.
	uint16_t pciSegmentGroupNumber;
	
	// Start PCI bus number decoded by this host bridge.
	uint8_t startPciBusNumber;
	
	// End PCI bus number decoded by this host bridge.
	uint8_t endPciBusNumber;
	
	// Reserved.
	uint32_t reserved;
	
} __attribute__((__packed__)) mcfg_entry_t;

// Structure of the MCFG table.
typedef struct
{
	// Table header.
	acpi_header_t header;
	
	// Reserved.
	uint64_t reserved;
	
	// The entries (each is 16 bytes large, the count can be determined using the MCFG table size).
	mcfg_entry_t entries[1];
} __attribute__((__packed__)) mcfg_t;

void mcfg_scan(mcfg_t *mcfg);

#endif
