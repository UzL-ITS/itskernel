#pragma once

#include <stdint.h>
#include <acpi/common.h>

#define FADT_SIGNATURE 0x50434146 // 'FACP'

// FADT address types.
typedef enum
{
	// Access via memory.
	FADT_ADDR_TYPE_SYSTEM_MEMORY = 0,
	
	// Access via CPU port.
	FADT_ADDR_TYPE_SYSTEM_IO = 1,
	
	// Access via PCI configuration space.
	FADT_ADDR_TYPE_PCI_CFG_SPACE = 2
	
} fadt_addr_type_t;

// FADT register address entry.
typedef struct
{
	// Address type (see fadt_addr_type_t).
	uint8_t addressSpace;
	
	// Register bit width.
	uint8_t bitWidth;
	
	// The register bit offset.
	uint8_t bitOffset;
	
	// Target size.
	// 1: 8-bit access
	// 2: 16-bit access
	// 3: 32-bit access
	// 4: 64-bit access
	uint8_t accessSize;
	
	// Address.
	uint64_t address;
	
} __attribute__((__packed__)) fadt_addr_struct_t;

// Structure of the FADT table.
typedef struct
{
	// Table header.
	acpi_header_t header;
	
	
	uint32_t firmwareCtrl;
	uint32_t dsdt;
 
	// Unused
	uint8_t Reserved;

	uint8_t preferredPowerManagementProfile;
	uint16_t sciInterrupt;
	uint32_t smiCommandPort;
	uint8_t acpiEnable;
	uint8_t acpiDisable;
	uint8_t s4BIOS_REQ;
	uint8_t pState_Control;
	uint32_t pM1aEventBlock;
	uint32_t pM1bEventBlock;
	uint32_t pM1aControlBlock;
	uint32_t pM1bControlBlock;
	uint32_t pM2ControlBlock;
	uint32_t pMTimerBlock;
	uint32_t gPE0Block;
	uint32_t gPE1Block;
	uint8_t pM1EventLength;
	uint8_t pM1ControlLength;
	uint8_t pM2ControlLength;
	uint8_t pMTimerLength;
	uint8_t gPE0Length;
	uint8_t gPE1Length;
	uint8_t gPE1Base;
	uint8_t cStateControl;
	uint16_t worstC2Latency;
	uint16_t worstC3Latency;
	uint16_t flushSize;
	uint16_t flushStride;
	uint8_t dutyOffset;
	uint8_t dutyWidth;
	uint8_t dayAlarm;
	uint8_t monthAlarm;
	uint8_t century;

	uint16_t bootArchitectureFlags;

	uint8_t reserved2;
	uint32_t flags;

	fadt_addr_struct_t resetReg;

	uint8_t resetValue;
	uint8_t reserved3[3];

	uint64_t x_FirmwareControl;
	uint64_t x_Dsdt;

	fadt_addr_struct_t x_PM1aEventBlock;
	fadt_addr_struct_t x_PM1bEventBlock;
	fadt_addr_struct_t x_PM1aControlBlock;
	fadt_addr_struct_t x_PM1bControlBlock;
	fadt_addr_struct_t x_PM2ControlBlock;
	fadt_addr_struct_t x_PMTimerBlock;
	fadt_addr_struct_t x_GPE0Block;
	fadt_addr_struct_t x_GPE1Block;
	
} __attribute__((__packed__)) fadt_t;

// Scans the FADT table and initializes depending components.
void fadt_scan(fadt_t *fadt);

