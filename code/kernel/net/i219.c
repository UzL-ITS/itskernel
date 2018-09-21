/*
Intel i219 (e1000e) ethernet driver.
*/

#include <net/i219.h>
#include <trace/trace.h>
#include <mm/mmio.h>
#include <panic/panic.h>
#include <stdlib/stdlib.h>
#include <stdlib/string.h>
#include <mm/heap.h>
#include <cpu/pause.h>

// Maximum Transmission Unit (this value is slightly arbitrary, it matches the value used in the user-space LWIP wrapper).
#define I219_MTU 1522

// Device register offsets.
typedef enum
{
	I219_REG_CTRL = 0x0000, // Control Register
	I219_REG_STATUS = 0x0008, // Device Status Register
	I82542_REG_EERD = 0x0014, // EPROM Read Register
	I219_REG_CTRLEXT = 0x0018, // Extended Control Register
	I219_REG_MDIC = 0x0020, // MDI Control Register
	I219_REG_FCAL = 0x0028, // Flow Control Address Low
	I219_REG_FCAH = 0x002C, // Flow Control Address High
	I219_REG_FCT = 0x0030, // Flow Control Type
	I219_REG_VET = 0x0038, // VLAN Ether Type
	I219_REG_ICR = 0x00C0, // Interrupt Cause Read
	I219_REG_ITR = 0x00C4, // Interrupt Throttling Register
	I219_REG_ICS = 0x00C8, // Interrupt Cause Set Register
	I219_REG_IMS = 0x00D0, // Interrupt Mask Set/Read Register
	I219_REG_IMC = 0x00D8, // Interrupt Mask Clear Register
	I219_REG_RCTL = 0x0100, // Receive Control Register
	I219_REG_FCTTV = 0x0170, // Flow Control Transmit Timer Value
	I219_REG_TXCW = 0x0178, // Transmit Configuration Word
	I219_REG_RXCW = 0x0180, // Receive Configuration Word
	I219_REG_TCTL = 0x0400, // Transmit Control Register
	I219_REG_TIPG = 0x0410, // Transmit Inter Packet Gap
	I219_REG_LEDCTL = 0x0E00, // LED Control
	I219_REG_PBA = 0x1000, // Packet Buffer Allocation
	I219_REG_RDBAL = 0x2800, // RX Descriptor Base Address Low
	I219_REG_RDBAH = 0x2804, // RX Descriptor Base Address High
	I219_REG_RDLEN = 0x2808, // RX Descriptor Length
	I219_REG_RDH = 0x2810, // RX Descriptor Head
	I219_REG_RDT = 0x2818, // RX Descriptor Tail
	I219_REG_RDTR = 0x2820, // RX Delay Timer Register
	I219_REG_RXDCTL = 0x3828, // RX Descriptor Control
	I219_REG_RADV = 0x282C, // RX Int. Absolute Delay Timer
	I219_REG_RSRPD = 0x2C00, // RX Small Packet Detect Interrupt
	I219_REG_TXDMAC = 0x3000, // TX DMA Control
	I219_REG_TDBAL = 0x3800, // TX Descriptor Base Address Low
	I219_REG_TDBAH = 0x3804, // TX Descriptor Base Address High
	I219_REG_TDLEN = 0x3808, // TX Descriptor Length
	I219_REG_TDH = 0x3810, // TX Descriptor Head
	I219_REG_TDT = 0x3818, // TX Descriptor Tail
	I219_REG_TIDV = 0x3820, // TX Interrupt Delay Value
	I219_REG_TXDCTL = 0x3828, // TX Descriptor Control
	I219_REG_TADV = 0x382C, // TX Absolute Interrupt Delay Value
	I219_REG_TSPMT = 0x3830, // TCP Segmentation Pad & Min Threshold
	I219_REG_RXCSUM = 0x5000, // RX Checksum Control
	I219_REG_MTA = 0x5200, // Multicast Table Array
	I219_REG_RAL = 0x5400, // Receive Address Low
	I219_REG_RAH = 0x5404, // Receive Address High
	
	I219_REG_GPRC = 0x4074, // Good Packets Received Count
	I219_REG_BPRC = 0x4078, // Broadcast Packets Received Count
	I219_REG_MPRC = 0x407C, // Multicast Packets Received Count
} i219_register_t;

// Interrupt types.
typedef enum
{
	I219_INTR_TXDW = 0x0001,
	I219_INTR_TXQE = 0x0002,
	I219_INTR_LSC = 0x0004,
	I219_INTR_RXSEQ = 0x0008,
	I219_INTR_RXDMT0 = 0x0010,
	I219_INTR_RXO = 0x0040,
	I219_INTR_RXT0 = 0x0080,
	I219_INTR_MDAC = 0x0200,
	I219_INTR_RXCFG = 0x0400,
	I219_INTR_PHYINT = 0x1000,
	I219_INTR_GDISDP6 = 0x2000,
	I219_INTR_GDISDP7 = 0x4000,
	I219_INTR_TXDLOW = 0x8000
} i219_interrupt_t;

// Structure of receive descriptors.
typedef struct
{
	// Physical address of the receive descriptor packet buffer.
	uint64_t address;
	
	// Length of the received packet part.
	uint16_t length;
	
	// Checksum of the received packet part.
	uint16_t checksum;
	
	// Packet reception status.
	uint8_t status;
	
	// Packet reception errors.
	uint8_t errors;
	
	// Unused.
	uint16_t special;
} __attribute__((__packed__)) rx_desc_t;

// Structure of transmit descriptors.
typedef struct
{
	// Physical address of the transmit descriptor packet buffer.
	uint64_t address;
	
	// Length of the packet part to transmit.
	uint16_t length;
	
	// Check sum offset, unused (set to 0).
	uint8_t cso;
	
	// Command field.
	uint8_t command;
	
	// Packet transmission status.
	uint8_t status;
	
	// Checksum start field, unused.
	uint8_t css;
	
	// Unused.
	uint16_t special;
} __attribute__((__packed__)) tx_desc_t;

// Structure of a receive packet list entry.
typedef struct received_packet_s
{
	// The next received packet list entry.
	struct received_packet_s *next;
	
	// The length of the received packet.
	int length;
	
	// Packet data
	uint8_t packet[I219_MTU];
} received_packet_t;


// Pointer to BAR0 MMIO memory.
static uint8_t *deviceBar0Memory = 0;

// Size of BAR0 MMIO memory.
static uint64_t deviceBar0MemorySize;

// The card's MAC address.
static uint8_t macAddress[6];

// Pointer to the receive descriptor ring buffer.
#define RX_DESC_COUNT 256
static rx_desc_t *rxDescriptors;

// Virtual base address of the receive buffer memory.
#define RX_BUFFER_SIZE 2048
static uint8_t *rxBufferMem;

// Pointer to the transmit descriptor ring buffer.
#define TX_DESC_COUNT 256
static tx_desc_t *txDescriptors;

// Virtual base address of the transmit buffer memory.
#define TX_BUFFER_SIZE 2048
static uint8_t *txBufferMem;

// The current receive buffer tail.
static int rxTail;

// The current send buffer tail.
static int txTail;

// Start and end nodes of the received packets queue.
// The first element is the oldest, the last the newest (FIFO).
static received_packet_t *receivedPacketsQueueStart;
static received_packet_t *receivedPacketsQueueEnd;

// Start node of the received packets buffer list.
static received_packet_t *receivedPacketsBufferListStart;


// Reads the given device register using MMIO.
static uint32_t i219_read(i219_register_t reg)
{
	return *((uint32_t *)(deviceBar0Memory + reg));
}

// Sets a new value for the given device register using MMIO.
static void i219_write(i219_register_t reg, uint32_t value)
{
	*((uint32_t *)(deviceBar0Memory + reg)) = value;
}

void i219_init(pci_cfgspace_header_0_t *deviceCfgSpaceHeader)
{
	// Map BAR0 memory area
	pci_bar_info_t bar0Info;
	pci_get_bar_info(deviceCfgSpaceHeader, 0, &bar0Info);
	deviceBar0MemorySize = bar0Info.size;
	deviceBar0Memory = (uint8_t *)mmio_map(bar0Info.baseAddress, bar0Info.size, VM_R | VM_W);
	if(!deviceBar0Memory)
		panic("Error: Could not map Intel i219 BAR0 MMIO.");
	
	// Enable PCI bus mastering (else no PCI accesses possible)
	uint32_t commandRegister = deviceCfgSpaceHeader->commonHeader.command;
	commandRegister |= 0x04;
	deviceCfgSpaceHeader->commonHeader.command = commandRegister;
	
	// Read MAC address
	uint32_t macLow = i219_read(I219_REG_RAL);
	if(macLow != 0x00000000)
	{
		// MAC can be read from RAL[0]/RAH[0] MMIO directly
		macAddress[0] = macLow & 0xFF;
		macAddress[1] = (macLow >> 8) & 0xFF;
		macAddress[2] = (macLow >> 16) & 0xFF;
		macAddress[3] = (macLow >> 24) & 0xFF;
		uint32_t macHigh = i219_read(I219_REG_RAH);
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
		macAddress[0],
		macAddress[1],
		macAddress[2],
		macAddress[3],
		macAddress[4],
		macAddress[5]
	);
	trace_printf("Device status: %08x\n", i219_read(I219_REG_STATUS));
	
	// Disable and clear all pending interrupts
	i219_write(I219_REG_IMC, 0xFFFFFFFF);
	i219_read(I219_REG_ICR);
	
	// Interrupt throttling: Wait 1000 * 256ns = 256us between interrupts
	// TODO adjust this for performance optimization
	//i219_write(I219_REG_ITR, 1000);
	i219_write(I219_REG_ITR, 0);
	// TODO is disabled for now to simplify receive packet handling logic - else the interrupt handler needed to process multiple packets at once
	
	// Device control register
	uint32_t ctrl = i219_read(I219_REG_CTRL);
	ctrl &= ~0x00000008; // LRST = 0 (disable reset)
	ctrl |=  0x00000020; // ASDE = 1 (auto speed detection enable)
	ctrl |=  0x00000040; // SLU = 1 (set link up)
	i219_write(I219_REG_CTRL, ctrl);
	
	// Clear multicast table array
	for(int i = 0; i < 128; ++i)
		i219_write(I219_REG_MTA + 4 * i, 0x00000000);
	
	// Allocate receive data buffer
	uint64_t rxBufferMemPhy;
	rxBufferMem = heap_alloc_contiguous(RX_DESC_COUNT * RX_BUFFER_SIZE, VM_R | VM_W, &rxBufferMemPhy);
	if(!rxBufferMem)
		panic("Could not allocate i219 receive data buffer.");
	
	// Allocate and initialize receive descriptor buffer
	uint64_t rxDescriptorsPhy;
	rxDescriptors = heap_alloc_contiguous(RX_DESC_COUNT * sizeof(rx_desc_t), VM_R | VM_W, &rxDescriptorsPhy);
	if(!rxDescriptors)
		panic("Could not allocate i219 receive descriptor buffer.");
	for(int i = 0; i < RX_DESC_COUNT; ++i)
	{
		// Initialize descriptor
		rx_desc_t *currDesc = &rxDescriptors[i];
		currDesc->address = rxBufferMemPhy + i * RX_BUFFER_SIZE;
		currDesc->status = 0;
	}
	
	// Pass receive descriptor buffer
	trace_printf("rxDescriptorsPhy = %012x\n", rxDescriptorsPhy);
	trace_printf("rxBufferMemPhy = %012x\n", rxBufferMemPhy);
	i219_write(I219_REG_RDBAH, rxDescriptorsPhy >> 32);
	i219_write(I219_REG_RDBAL, rxDescriptorsPhy & 0xFFFFFFFF);
	i219_write(I219_REG_RDLEN, RX_DESC_COUNT * sizeof(rx_desc_t));
	i219_write(I219_REG_RDH, 0);
	i219_write(I219_REG_RDT, RX_DESC_COUNT - 1);
	rxTail = RX_DESC_COUNT - 1;
	
	// Allocate transmit data buffer
	uint64_t txBufferMemPhy;
	txBufferMem = heap_alloc_contiguous(TX_DESC_COUNT * TX_BUFFER_SIZE, VM_R | VM_W, &txBufferMemPhy);
	if(!txBufferMem)
		panic("Could not allocate i219 transmit data buffer.");
	
	// Allocate and initialize transmit descriptor buffer
	uint64_t txDescriptorsPhy;
	txDescriptors = heap_alloc_contiguous(TX_DESC_COUNT * sizeof(tx_desc_t), VM_R | VM_W, &txDescriptorsPhy);
	if(!txDescriptors)
		panic("Could not allocate i219 transmit descriptor buffer.");
	for(int i = 0; i < TX_DESC_COUNT; ++i)
	{
		// Initialize descriptor
		tx_desc_t *currDesc = &txDescriptors[i];
		currDesc->address = txBufferMemPhy + i * TX_BUFFER_SIZE;
		currDesc->length = 0;
		currDesc->status = 0;
		currDesc->cso = 0;
		currDesc->css = 0;
		currDesc->special = 0;
	}
	
	// Pass transmit descriptor buffer
	trace_printf("txDescriptorsPhy = %012x\n", txDescriptorsPhy);
	trace_printf("txBufferMemPhy = %012x\n", txBufferMemPhy);
	i219_write(I219_REG_TDBAH, txDescriptorsPhy >> 32);
	i219_write(I219_REG_TDBAL, txDescriptorsPhy & 0xFFFFFFFF);
	i219_write(I219_REG_TDLEN, TX_DESC_COUNT * sizeof(tx_desc_t));
	i219_write(I219_REG_TDH, 0);
	i219_write(I219_REG_TDT, 0);
	txTail = 0;
	
	// Transmit IPG: Use recommended values 10, 8 and 6
	i219_write(I219_REG_TIPG, (6 << 20) | (8 << 10) | 10);
	
	// Enable transmitter
	uint32_t tctl = 0;
	tctl |= 0x00000002; // EN (Transmitter Enable)
	tctl |= 0x00000008; // PSP (Pad Short Packets)
	tctl |= 0x000000F0; // 16 retries
	tctl |= 0x0003F000; // 64-byte Collision Distance
	tctl |= 0x01000000; // RTLC (Re-transmit on Late Collision)
	tctl |= 0x10000000; // Reserved (must be 1)
	tctl |= 0x20000000; // 64-byte Read Request Threshold
	i219_write(I219_REG_TCTL, tctl);
	
	// Enable receiver
	uint32_t rctl = 0;
	rctl |= 0x00000002; // EN (Receiver Enable)
	rctl |= 0x00000004; // SBP (Store Pad Packets)
	//rctl |= 0x00000020; // LPE (Long Packet Reception Enable)   -> MTU is set to 1522, thus we don't use this feature
	rctl |= 0x00008000; // BAM (Broadcast Accept Mode)
	rctl |= 0x00000000; // BSIZE = 2048 (Receive Buffer Size)
	//rctl |= 0x02000000; // BSEX (Buffer Size Extension)
	rctl |= 0x04000000; // SECRC (Strip Ethernet CRC)
	//rctl |= 0x00000018; // UPE+MPE (Promiscuous mode)           -> for testing only!
	i219_write(I219_REG_RCTL, rctl);
	
	// Pre-allocate some buffers for the received packets list
	receivedPacketsQueueStart = 0;
	receivedPacketsQueueEnd = 0;
	receivedPacketsBufferListStart = 0;
	for(int i = 0; i < RX_DESC_COUNT; ++i)
	{
		// Allocate buffer entry
		received_packet_t *bufferEntry = (received_packet_t *)malloc(sizeof(received_packet_t));
		
		// Set pointers
		bufferEntry->next = receivedPacketsBufferListStart;
		receivedPacketsBufferListStart = bufferEntry;
	}
	
	// Enable all interrupts
	i219_write(I219_REG_IMS, 0x1F6DC);
	
	trace_printf("Network driver initialized.\n");
}

void i219_get_mac_address(uint8_t *macBuffer)
{
	// Copy MAC address
	for(int i = 0; i < 6; ++i)
		macBuffer[i] = macAddress[i];
}

void i219_send(uint8_t *packet, int packetLength)
{
	//trace_printf("Sending packet with length %d\n", packetLength);
	
	/*trace_printf("First bytes: ");
	int debugLen = (packetLength > 32 ? 32 : packetLength);
	for(int i = 0; i < debugLen; ++i)
		trace_printf("%02x ", packet[i]);
	trace_printf("...\n");*/
	
	// Wait until descriptor at current tail is unused
	tx_desc_t *desc = &txDescriptors[txTail];
	if(desc->length)
		while(!(desc->status & 0xF)) // Catches "descriptor done" (DD) and various errors
			pause_once();
	//trace_printf("Exit send wait loop\n");
	
	// Copy packet data
	memcpy(&txBufferMem[txTail * TX_BUFFER_SIZE], packet, packetLength);
	desc->length = packetLength;
	desc->command = 0x8 | 0x2 | 0x1; // RS (Report Status), IFCS (Insert FCS), EOP (End Of Packet)
	desc->cso = 0;
	desc->status = 0;
	desc->css = 0;
	desc->special = 0;
	
	// Update tail
	++txTail;
	if(txTail == TX_DESC_COUNT)
		txTail = 0;
	i219_write(I219_REG_TDT, txTail);
	
	//trace_printf("Passing packet to device done.\n");
}

// Processes one received packet.
// TODO use ITR (interrupt throttling register) to fire interrupts for multiple packets at once (less interrupts)
static void i219_receive()
{
	// Receive multiple packets
	for(int p = 0; p < RX_DESC_COUNT; ++p)
	{
		// Packet received?
		int newRxTail = rxTail + 1;
		if(newRxTail == RX_DESC_COUNT)
			newRxTail = 0;
		if(rxDescriptors[newRxTail].status & 1)
		{
			// Retrieve packet
			uint8_t *packet = &rxBufferMem[newRxTail * RX_BUFFER_SIZE];
			int packetLength = rxDescriptors[newRxTail].length;
			
			trace_printf("Received descriptor with length %d\n", packetLength);
			trace_printf("First bytes: ");
			int debugLen = (packetLength > 32 ? 32 : packetLength);
			for(int i = 0; i < debugLen; ++i)
				trace_printf("%02x ", packet[i]);
			trace_printf("...\n");
			
			// Errors?
			if(rxDescriptors[newRxTail].errors)
				trace_printf("Error byte in descriptor: %02x\n", rxDescriptors[newRxTail].errors);
			else if(packetLength > I219_MTU)
				panic("Received packet size %d exceeds assumed MTU %d!\n", packetLength, I219_MTU);
			else
			{
				// Allocate a receive buffer list entry
				received_packet_t *bufferEntry;
				if(receivedPacketsBufferListStart)
				{
					// Use pre-allocated buffer entry
					bufferEntry = receivedPacketsBufferListStart;
					receivedPacketsBufferListStart = bufferEntry->next;
				}
				else
				{
					// Allocate new buffer entry
					bufferEntry = (received_packet_t *)malloc(sizeof(received_packet_t));
				}
				
				// Copy data
				bufferEntry->length = packetLength;
				memcpy(bufferEntry->packet, packet, packetLength);
				
				// Insert entry into list
				if(!receivedPacketsQueueStart)
					receivedPacketsQueueStart = bufferEntry;
				if(receivedPacketsQueueEnd)
					receivedPacketsQueueEnd->next = bufferEntry;
				receivedPacketsQueueEnd = bufferEntry;
				bufferEntry->next = 0;
			}
			
			// Reset status
			rxDescriptors[newRxTail].status = 0;
			
			// Update receive tail
			rxTail = newRxTail;
			i219_write(I219_REG_RDT, rxTail);
		}
		else
			break; // No more received packets
	}
}

int i219_next_received_packet(uint8_t *packetBuffer)
{
	// Any packet available?
	if(!receivedPacketsQueueStart)
		return 0;
	
	// Remove packet buffer entry from queue
	received_packet_t *bufferEntry = receivedPacketsQueueStart;
	receivedPacketsQueueStart = bufferEntry->next;
	if(!receivedPacketsQueueStart)
		receivedPacketsQueueEnd = 0;
	
	// Copy entry contents
	int packetLength = bufferEntry->length;
	memcpy(packetBuffer, bufferEntry->packet, packetLength);
	
	// Mark buffer as free
	bufferEntry->next = receivedPacketsBufferListStart;
	receivedPacketsBufferListStart = bufferEntry;
	
	// Done
	return packetLength;
}

bool i219_handle_interrupt(cpu_state_t *state)
{
	// Ensure initialization is done
	if(!deviceBar0Memory)
		return false;
	
	// Read interrupt cause register
	uint32_t icr = i219_read(I219_REG_ICR);
	if(!icr)
		return false;
	
	// Handle set interrupts
	//trace_printf("Intel8254x interrupt! ICR: %08x\n", icr);
	if(icr & I219_INTR_RXT0)
	{
		// Receive timer expired, handle received packets
		i219_receive();
	}
	return true;
}