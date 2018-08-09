
#ifndef _PCI_H
#define _PCI_H

#include <stdint.h>

// Information shared by all PCI configuration space headers.
typedef struct
{
	// ID of the device vendor.
	uint16_t vendorId;
	
	// ID of the device.
	uint16_t deviceId;
	
	// Command register.
	uint16_t command;
	
	// Status register.
	uint16_t status;
	
	// Device revision ID.
	uint8_t revisionId;
	
	// The register-level programming interface.
	uint8_t progIf;
	
	// The device sub class code.
	uint8_t subclassCode;
	
	// The device class code.
	uint8_t classCode;
	
	// Legacy field, probably unused.
	uint8_t cacheLineSize;
	
	// Does not apply, unused.
	uint8_t masterLatencyTimer;
	
	// The associated header type.
	uint8_t headerType;
	
	// Control of built-in self test.
	uint8_t bist;
	
} __attribute__((__packed__)) pci_cfgspace_header_common_t;

// PCI configuration space type 0 header.
typedef struct
{
	// Common header.
	pci_cfgspace_header_common_t commonHeader;
	
	// Base address registers (BARs).
	union
	{
		uint32_t bars32[6];
		uint64_t bars64[3];
	} bars;
	
	// (?) CardBus related, unused here.
	uint32_t cardbusCisPointer;
	
	// (?) Unused here.
	uint16_t subsystemVendorId;
	
	// (?) Unused here.
	uint16_t subsystemId;
	
	// (?) Unused here.
	uint32_t expansionRomBaseAddress;
	
	// Points to the first device capabilities list entry.
	uint8_t capabilitiesPointer;
	
	// Reserved.
	uint8_t reserved[7];
	
	
	uint8_t interruptLine;
	
	uint8_t interruptPin;
	
	// Unused.
	uint8_t minGnt;
	
	// Unused.
	uint8_t maxLat;
} __attribute__((__packed__)) pci_cfgspace_header_0_t;

// Adds the given memory mapped device configuration space to the internal device list.
void pci_add_discovered_device(pci_cfgspace_header_common_t *deviceCfgSpaceHeaderCommon);

// Initializes the PCI handler.
void pci_init();



#endif