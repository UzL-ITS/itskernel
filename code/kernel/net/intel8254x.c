/*
Intel 8254x ethernet driver.
*/

#include <net/intel8254x.h>
#include <trace/trace.h>
#include <mm/mmio.h>
#include <panic/panic.h>
#include <stdlib/stdlib.h>

// Device register offsets.
typedef enum
{
	I8254X_REG_CTRL = 0x0000, // Control Register
	I8254X_REG_STATUS = 0x0008, // Device Status Register
	I82542_REG_EERD = 0x0014, // EPROM Read Register
	I8254X_REG_CTRLEXT = 0x0018, // Extended Control Register
	I8254X_REG_MDIC = 0x0020, // MDI Control Register
	I8254X_REG_FCAL = 0x0028, // Flow Control Address Low
	I8254X_REG_FCAH = 0x002C, // Flow Control Address High
	I8254X_REG_FCT = 0x0030, // Flow Control Type
	I8254X_REG_VET = 0x0038, // VLAN Ether Type
	I8254X_REG_ICR = 0x00C0, // Interrupt Cause Read
	I8254X_REG_ITR = 0x00C4, // Interrupt Throttling Register
	I8254X_REG_ICS = 0x00C8, // Interrupt Cause Set Register
	I8254X_REG_IMS = 0x00D0, // Interrupt Mask Set/Read Register
	I8254X_REG_IMC = 0x00D8, // Interrupt Mask Clear Register
	I8254X_REG_RCTL = 0x0100, // Receive Control Register
	I8254X_REG_FCTTV = 0x0170, // Flow Control Transmit Timer Value
	I8254X_REG_TXCW = 0x0178, // Transmit Configuration Word
	I8254X_REG_RXCW = 0x0180, // Receive Configuration Word
	I8254X_REG_TCTL = 0x0400, // Transmit Control Register
	I8254X_REG_TIPG = 0x0410, // Transmit Inter Packet Gap
	I8254X_REG_LEDCTL = 0x0E00, // LED Control
	I8254X_REG_PBA = 0x1000, // Packet Buffer Allocation
	I8254X_REG_RDBAL = 0x2800, // RX Descriptor Base Address Low
	I8254X_REG_RDBAH = 0x2804, // RX Descriptor Base Address High
	I8254X_REG_RDLEN = 0x2808, // RX Descriptor Length
	I8254X_REG_RDH = 0x2810, // RX Descriptor Head
	I8254X_REG_RDT = 0x2818, // RX Descriptor Tail
	I8254X_REG_RDTR = 0x2820, // RX Delay Timer Register
	I8254X_REG_RXDCTL = 0x3828, // RX Descriptor Control
	I8254X_REG_RADV = 0x282C, // RX Int. Absolute Delay Timer
	I8254X_REG_RSRPD = 0x2C00, // RX Small Packet Detect Interrupt
	I8254X_REG_TXDMAC = 0x3000, // TX DMA Control
	I8254X_REG_TDBAL = 0x3800, // TX Descriptor Base Address Low
	I8254X_REG_TDBAH = 0x3804, // TX Descriptor Base Address High
	I8254X_REG_TDLEN = 0x3808, // TX Descriptor Length
	I8254X_REG_TDH = 0x3810, // TX Descriptor Head
	I8254X_REG_TDT = 0x3818, // TX Descriptor Tail
	I8254X_REG_TIDV = 0x3820, // TX Interrupt Delay Value
	I8254X_REG_TXDCTL = 0x3828, // TX Descriptor Control
	I8254X_REG_TADV = 0x382C, // TX Absolute Interrupt Delay Value
	I8254X_REG_TSPMT = 0x3830, // TCP Segmentation Pad & Min Threshold
	I8254X_REG_RXCSUM = 0x5000, // RX Checksum Control
	I8254X_REG_MTA = 0x5200, // Multicast Table Array
	I8254X_REG_RAL = 0x5400, // Receive Address Low
	I8254X_REG_RAH = 0x5404, // Receive Address High
} intel8254_register_t;

// Pointer to BAR0 MMIO memory.
static uint8_t *deviceBar0Memory;

// Size of BAR0 MMIO memory.
static uint64_t deviceBar0MemorySize;

// The card's MAC address.
static uint8_t macAddress[6];




// Reads the given device register using MMIO.
static uint32_t intel8254x_read(intel8254_register_t reg)
{
	return *((uint32_t *)(deviceBar0Memory + reg));
}

// Sets a new value for the given device register using MMIO.
static void intel8254x_write(intel8254_register_t reg, uint32_t value)
{
	*((uint32_t *)(deviceBar0Memory + reg)) = value;
}

void intel8254x_init(pci_cfgspace_header_0_t *deviceCfgSpaceHeader)
{
	// Map BAR0 memory area
	pci_bar_info_t bar0Info;
	pci_get_bar_info(deviceCfgSpaceHeader, 0, &bar0Info);
	deviceBar0MemorySize = bar0Info.size;
	deviceBar0Memory = (uint8_t *)mmio_map(bar0Info.baseAddress, bar0Info.size, VM_R | VM_W);
	if(!deviceBar0Memory)
		panic("Error: Could not map Intel 8254x BAR0 MMIO.");
	
	// Enable PCI bus mastering (else no PCI accesses possible)
	uint32_t commandRegister = deviceCfgSpaceHeader->commonHeader.command;
	commandRegister |= 0x04;
	deviceCfgSpaceHeader->commonHeader.command = commandRegister;
	
	// Read MAC address
	uint32_t macLow = intel8254x_read(I8254X_REG_RAL);
	if(macLow != 0x00000000)
	{
		// MAC can be read from RAL[0]/RAH[0] MMIO directly
		macAddress[0] = macLow & 0xFF;
		macAddress[1] = (macLow >> 8) & 0xFF;
		macAddress[2] = (macLow >> 16) & 0xFF;
		macAddress[3] = (macLow >> 24) & 0xFF;
		uint32_t macHigh = intel8254x_read(I8254X_REG_RAH);
		macAddress[4] = macHigh & 0xFF;
		macAddress[5] = (macHigh >> 8) & 0xFF;
	}
	else
	{
		// We have to use EEPROM
		panic("Implement 8254x EEPROM reading (register 0x14)");
		// also write these MAC into RAL[0]/RAH[0]
	}
	trace_printf("MAC address: %02x:%02x:%02x:%02x:%02x:%02x\n",
		macAddress[5],
		macAddress[4],
		macAddress[3],
		macAddress[2],
		macAddress[1],
		macAddress[0]
	);
	
	// Disable and clear all pending interrupts
	intel8254x_write(I8254X_REG_IMC, 0xFFFFFFFF);
	intel8254x_read(I8254X_REG_ICR);
	
	// Interrupt throttling: Wait 1000 * 256ns = 256us between interrupts
	// TODO adjust this for performance optimization
	intel8254x_write(I8254X_REG_ITR, 1000);
	
	// TXCW nötig??
	
	// Device control register
	uint32_t ctrl = intel8254x_read(I8254X_REG_CTRL);
	ctrl &= ~0x00000008; // LRST = 0 (disable reset
	ctrl |=  0x00000020; // ASDE = 1 (auto speed detection enable)
	ctrl |=  0x00000040; // SLU = 1 (set link up)
	intel8254x_write(I8254X_REG_CTRL, ctrl);
	
	// Clear multicast table array
	for(int i = 0; i < 128; ++i)
		intel8254x_write(I8254X_REG_MTA + 4 * i, 0x00000000);
	
	// TODO continue here with heap_alloc_contiguous
}