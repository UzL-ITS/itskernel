#pragma once

#include <stdint.h>
#include <stdbool.h>


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
	
	// Unreliable.
	uint8_t interruptLine;
	
	// Unreliable.
	uint8_t interruptPin;
	
	// Unused.
	uint8_t minGnt;
	
	// Unused.
	uint8_t maxLat;
} __attribute__((__packed__)) pci_cfgspace_header_0_t;

// Capability header.
typedef struct
{
	// Capability ID.
	uint8_t capId;
	
	// Next capability offset.
	uint8_t nextCap;
	
} __attribute__((__packed__)) pci_cap_common_t;

typedef struct
{
	// The BAR's base address.
	uint64_t baseAddress;
	
	// The bitness of the BAR's base address.
	uint8_t bitness;
	
	// Tells whether the bar points to MMIO (true) or I/O space (false).
	bool isMmio;
	
	// If the bar points to MMIO, this tells whether the memory is considered prefetchable or not.
	bool isPrefetchable;
	
	// The size of the BAR in bytes.
	uint64_t size;
} pci_bar_info_t;

// Adds the given memory mapped device configuration space to the internal device list.
void pci_add_discovered_device(pci_cfgspace_header_common_t *deviceCfgSpaceHeaderCommon);

// Retrieves the information of the given PCI BAR.
void pci_get_bar_info(pci_cfgspace_header_0_t *deviceCfgSpaceHeader, int barIndex, pci_bar_info_t *infoPtr);

// Initializes the PCI handler.
void pci_init();
