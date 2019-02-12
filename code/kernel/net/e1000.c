/*
Intel 8254x (e1000) ethernet driver.
*/

#include <net/e1000.h>
#include <net/e1000_defs.h>
#include <trace/trace.h>
#include <mm/mmio.h>
#include <panic/panic.h>
#include <stdlib/stdlib.h>
#include <stdlib/string.h>
#include <mm/heap.h>
#include <cpu/pause.h>

// Maximum Transmission Unit (this value is slightly arbitrary, it matches the value used in the user-space LWIP wrapper).
#define E1000_MTU 1522

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
	uint8_t packet[E1000_MTU];
} received_packet_t;


// Pointer to BAR0 MMIO memory.
static uint8_t *deviceBar0Memory;

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
static uint32_t e1000_read(e1000_register_t reg)
{
	return *((uint32_t *)(deviceBar0Memory + reg));
}

// Sets a new value for the given device register using MMIO.
static void e1000_write(e1000_register_t reg, uint32_t value)
{
	*((uint32_t *)(deviceBar0Memory + reg)) = value;
}

void e1000_init(pci_cfgspace_header_0_t *deviceCfgSpaceHeader)
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
	uint32_t macLow = e1000_read(E1000_REG_RAL);
	if(macLow != 0x00000000)
	{
		// MAC can be read from RAL[0]/RAH[0] MMIO directly
		macAddress[0] = macLow & 0xFF;
		macAddress[1] = (macLow >> 8) & 0xFF;
		macAddress[2] = (macLow >> 16) & 0xFF;
		macAddress[3] = (macLow >> 24) & 0xFF;
		uint32_t macHigh = e1000_read(E1000_REG_RAH);
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
	
	// Disable and clear all pending interrupts
	e1000_write(E1000_REG_IMC, 0xFFFFFFFF);
	e1000_read(E1000_REG_ICR);
	
	// Interrupt throttling: Wait 1000 * 256ns = 256us between interrupts
	// TODO adjust this for performance optimization
	//e1000_write(E1000_REG_ITR, 1000);
	e1000_write(E1000_REG_ITR, 0);
	// TODO is disabled for now to simplify receive packet handling logic - else the interrupt handler needed to process multiple packets at once
	
	// Device control register
	uint32_t ctrl = e1000_read(E1000_REG_CTRL);
	ctrl &= ~0x00000008; // LRST = 0 (disable reset)
	ctrl |=  0x00000020; // ASDE = 1 (auto speed detection enable)
	ctrl |=  0x00000040; // SLU = 1 (set link up)
	e1000_write(E1000_REG_CTRL, ctrl);
	
	// Clear multicast table array
	for(int i = 0; i < 128; ++i)
		e1000_write(E1000_REG_MTA + 4 * i, 0x00000000);
	
	// Allocate receive data buffer
	uint64_t rxBufferMemPhy;
	rxBufferMem = heap_alloc_contiguous(RX_DESC_COUNT * RX_BUFFER_SIZE, VM_R | VM_W, &rxBufferMemPhy);
	if(!rxBufferMem)
		panic("Could not allocate e1000 receive data buffer.");
	
	// Allocate and initialize receive descriptor buffer
	uint64_t rxDescriptorsPhy;
	rxDescriptors = heap_alloc_contiguous(RX_DESC_COUNT * sizeof(rx_desc_t), VM_R | VM_W, &rxDescriptorsPhy);
	if(!rxDescriptors)
		panic("Could not allocate e1000 receive descriptor buffer.");
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
	e1000_write(E1000_REG_RDBAH, rxDescriptorsPhy >> 32);
	e1000_write(E1000_REG_RDBAL, rxDescriptorsPhy & 0xFFFFFFFF);
	e1000_write(E1000_REG_RDLEN, RX_DESC_COUNT * sizeof(rx_desc_t));
	e1000_write(E1000_REG_RDH, 0);
	e1000_write(E1000_REG_RDT, RX_DESC_COUNT - 1);
	rxTail = RX_DESC_COUNT - 1;
	
	// Allocate transmit data buffer
	uint64_t txBufferMemPhy;
	txBufferMem = heap_alloc_contiguous(TX_DESC_COUNT * TX_BUFFER_SIZE, VM_R | VM_W, &txBufferMemPhy);
	if(!txBufferMem)
		panic("Could not allocate e1000 transmit data buffer.");
	
	// Allocate and initialize transmit descriptor buffer
	uint64_t txDescriptorsPhy;
	txDescriptors = heap_alloc_contiguous(TX_DESC_COUNT * sizeof(tx_desc_t), VM_R | VM_W, &txDescriptorsPhy);
	if(!txDescriptors)
		panic("Could not allocate e1000 transmit descriptor buffer.");
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
	e1000_write(E1000_REG_TDBAH, txDescriptorsPhy >> 32);
	e1000_write(E1000_REG_TDBAL, txDescriptorsPhy & 0xFFFFFFFF);
	e1000_write(E1000_REG_TDLEN, TX_DESC_COUNT * sizeof(tx_desc_t));
	e1000_write(E1000_REG_TDH, 0);
	e1000_write(E1000_REG_TDT, 0);
	txTail = 0;
	
	// Transmit IPG: Use recommended values 10, 8 and 6
	e1000_write(E1000_REG_TIPG, (6 << 20) | (8 << 10) | 10);
	
	// Enable transmitter
	uint32_t tctl = 0;
	tctl |= 0x00000002; // EN (Transmitter Enable)
	tctl |= 0x00000008; // PSP (Pad Short Packets)
	tctl |= 0x000000F0; // 16 retries
	tctl |= 0x00040000; // 64-byte Collision Distance
	tctl |= 0x01000000; // RTLC (Re-transmit on Late Collision)
	e1000_write(E1000_REG_TCTL, tctl);
	
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
	e1000_write(E1000_REG_RCTL, rctl);
	
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
	e1000_write(E1000_REG_IMS, 0x1F6DC);
	
	trace_printf("Network driver initialized.\n");
}

void e1000_get_mac_address(uint8_t *macBuffer)
{
	// Copy MAC address
	for(int i = 0; i < 6; ++i)
		macBuffer[i] = macAddress[i];
}

void e1000_send(uint8_t *packet, int packetLength)
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
	e1000_write(E1000_REG_TDT, txTail);
	
	//trace_printf("Passing packet to device done.\n");
}

// Processes one received packet.
// TODO use ITR (interrupt throttling register) to fire interrupts for multiple packets at once (less interrupts)
static void e1000_receive()
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
			
			//trace_printf("Received descriptor with length %d\n", packetLength);
			/*trace_printf("First bytes: ");
			int debugLen = (packetLength > 32 ? 32 : packetLength);
			for(int i = 0; i < debugLen; ++i)
				trace_printf("%02x ", packet[i]);
			trace_printf("...\n");*/
			
			// Errors?
			if(rxDescriptors[newRxTail].errors)
				trace_printf("Error byte in descriptor: %02x\n", rxDescriptors[newRxTail].errors);
			else if(packetLength > E1000_MTU)
				panic("Received packet size %d exceeds assumed MTU %d!\n", packetLength, E1000_MTU);
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
			e1000_write(E1000_REG_RDT, rxTail);
		}
		else
			break; // No more received packets
	}
}

int e1000_next_received_packet(uint8_t *packetBuffer)
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

bool e1000_handle_interrupt(cpu_state_t *state)
{
	// Read interrupt cause register
	uint32_t icr = e1000_read(E1000_REG_ICR);
	if(!icr)
		return false;
	
	// Handle set interrupts
	//trace_printf("Intel8254x interrupt! ICR: %08x\n", icr);
	if(icr & E1000_ICR_RXT0)
	{
		// Receive timer expired, handle received packets
		e1000_receive();
	}
	return true;
}