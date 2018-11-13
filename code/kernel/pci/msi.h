#pragma once

#include <stdint.h>
#include <pci/pci.h>
#include <intr/route.h>
#include <defs/types.h>

// PCI MSI capability structure header.
typedef struct
{
	// Capability header. ID must be 0x05.
	pci_cap_common_t capHeader;
	
	// Message control register.
	uint16_t messageControl;
	
} __attribute__((__packed__)) pci_cap_msi_header_t;

// PCI 32-bit MSI capability structure.
typedef struct
{
	// Shared MSI capability header.
	pci_cap_msi_header_t msiCapHeader;
	
	// Message address register.
	uint32_t messageAddress;
	
	// Message data register
	uint16_t messageData;
	
	// Reserved.
	// Access only when vector mode supported!
	uint16_t reserved;
	
	// Interrupt mask.
	// Access only when vector mode supported!
	uint32_t maskBits;
	
	// Pending interrupts.
	// Access only when vector mode supported!
	uint32_t pendingBits;
	
} __attribute__((__packed__)) pci_cap_msi32_t;

// PCI 64-bit MSI capability structure.
typedef struct
{
	// Shared MSI capability header.
	pci_cap_msi_header_t msiCapHeader;
	
	// Message address register (low).
	uint32_t messageAddressLow;
	
	// Message address register (high).
	uint32_t messageAddressHigh;
	
	// Message data register
	uint16_t messageData;
	
	// Reserved.
	// Access only when vector mode supported!
	uint16_t reserved;
	
	// Interrupt mask.
	// Access only when vector mode supported!
	uint32_t maskBits;
	
	// Pending interrupts.
	// Access only when vector mode supported!
	uint32_t pendingBits;
	
} __attribute__((__packed__)) pci_cap_msi64_t;

// Message control register flags.
typedef enum
{
	// Vector masking support.
	PCI_MSI_MESSAGE_CTRL_VECTOR_SUPPORTED = 0x0100,
	
	// 64-bit address support.
	PCI_MSI_MESSAGE_CTRL_64BIT_SUPPORTED = 0x0080,
	
	// MSI enable bit.
	PCI_MSI_MESSAGE_CTRL_ENABLE = 0x0001,
	
} pci_cap_msi_ctrl_flags_t;

// Enables MSI using the given device capability entry and interrupt handler.
void msi_enable(pci_cap_common_t *pciCapHeader, intr_t intr, intr_handler_t handler);