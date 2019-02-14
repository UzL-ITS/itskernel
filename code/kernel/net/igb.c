/*
Intel igb (I210/I211) ethernet driver.
*/

#include <net/igb.h>
#include <net/igb_defs.h>
#include <trace/trace.h>
#include <mm/mmio.h>
#include <panic/panic.h>
#include <stdlib/stdlib.h>
#include <stdlib/string.h>
#include <mm/heap.h>
#include <cpu/pause.h>
#include <lock/raw_spinlock.h>
#include <time/pit.h>

// Maximum Transmission Unit (this value is slightly arbitrary, it matches the value used in the user-space LWIP wrapper).
#define IGB_MTU 1522

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
	
	// VLAN tag.
	uint16_t vlan;
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
	
	// Reserved.
	uint8_t reserved;
	
	// VLAN tag.
	uint16_t vlan;
} __attribute__((__packed__)) tx_desc_t;

// Structure of a receive packet list entry.
typedef struct received_packet_s
{
	// The next received packet list entry.
	struct received_packet_s *next;
	
	// The length of the received packet.
	int length;
	
	// Packet data
	uint8_t packet[IGB_MTU];
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

// Determines whether initialization is done.
static bool initialized = false;

// Lock for the list of received packets.
static raw_spinlock_t receiveLock = RAW_SPINLOCK_UNLOCKED;


// Helper function for debugging. TODO remove
static void debug_regs();

// Reads the given device register using MMIO.
static uint32_t igb_read(e1000_register_t reg)
{
	return *((uint32_t *)(deviceBar0Memory + reg));
}

// Sets a new value for the given device register using MMIO.
static void igb_write(e1000_register_t reg, uint32_t value)
{
	*((uint32_t *)(deviceBar0Memory + reg)) = value;
}	

void igb_init(pci_cfgspace_header_0_t *deviceCfgSpaceHeader)
{
	// Map BAR0 memory area
	pci_bar_info_t bar0Info;
	pci_get_bar_info(deviceCfgSpaceHeader, 0, &bar0Info);
	deviceBar0MemorySize = bar0Info.size;
	deviceBar0Memory = (uint8_t *)mmio_map(bar0Info.baseAddress, bar0Info.size, VM_R | VM_W);
	if(!deviceBar0Memory)
		panic("Error: Could not map Intel igb BAR0 MMIO.");
	
	// Enable PCI bus mastering (else no PCI accesses possible)
	uint32_t commandRegister = deviceCfgSpaceHeader->commonHeader.command;
	commandRegister |= 0x07; // Make sure memory/IO space are also enabled
	deviceCfgSpaceHeader->commonHeader.command = commandRegister;
	
	// Perform software reset
	uint32_t ctrl = igb_read(IGB_REG_CTRL);
	igb_write(IGB_REG_CTRL, ctrl | IGB_CTRL_RST);
	pit_mdelay(5);
	uint32_t status;
	do
		status = igb_read(IGB_REG_STATUS);
	while(!(status & IGB_STATUS_PF_RST_DONE));
	trace_printf("IGB software reset completed\n");
	
	// Read MAC address
	uint32_t macLow = igb_read(IGB_REG_RAL);
	if(macLow != 0x00000000)
	{
		// MAC can be read from RAL[0]/RAH[0] MMIO directly
		macAddress[0] = macLow & 0xFF;
		macAddress[1] = (macLow >> 8) & 0xFF;
		macAddress[2] = (macLow >> 16) & 0xFF;
		macAddress[3] = (macLow >> 24) & 0xFF;
		uint32_t macHigh = igb_read(IGB_REG_RAH);
		macAddress[4] = macHigh & 0xFF;
		macAddress[5] = (macHigh >> 8) & 0xFF;
	}
	else
		panic("Could not read MAC address");
	
	trace_printf("MAC address: %02x:%02x:%02x:%02x:%02x:%02x\n",
		macAddress[0],
		macAddress[1],
		macAddress[2],
		macAddress[3],
		macAddress[4],
		macAddress[5]
	);
	trace_printf("INIT Device status: %08x\n", igb_read(IGB_REG_STATUS));
	trace_printf("     CTRL = %08x\n", igb_read(IGB_REG_CTRL));
	trace_printf("     RCTL = %08x\n", igb_read(IGB_REG_RCTL));
	trace_printf("     RDBAH = %08x\n", igb_read(IGB_REG_RDBAH));
	trace_printf("     RDBAL = %08x\n", igb_read(IGB_REG_RDBAL));
	trace_printf("     RDH = %08x\n", igb_read(IGB_REG_RDH));
	trace_printf("     RDT = %08x\n", igb_read(IGB_REG_RDT));
	trace_printf("     PBA = %08x\n", igb_read(IGB_REG_PBA));
	trace_printf("     PBS = %08x\n", igb_read(IGB_REG_PBS));
	trace_printf("     RFCTL = %08x\n", igb_read(IGB_REG_RFCTL));
	trace_printf("     RXCSUM = %08x\n", igb_read(IGB_REG_RXCSUM));
	trace_printf("     MRQC = %08x\n", igb_read(IGB_REG_MRQC));
	trace_printf("     TCTL = %08x\n", igb_read(IGB_REG_TCTL));
	
	// Disable all interrupts
	igb_write(IGB_REG_IMC, 0xFFFFFFFF);
	
	// Clear pending interrupts
	igb_read(IGB_REG_ICR);
	
	// Get hardware control from firmware
	/*uint32_t ctrlExt = igb_read(IGB_REG_CTRL_EXT);
	ctrlExt |= IGB_CTRL_EXT_DRV_LOAD;
	igb_write(IGB_REG_CTRL_EXT, ctrlExt);*/
	
	// Clear multicast table array
	for(int i = 0; i < 128; ++i)
		igb_write(IGB_REG_MTA + 4 * i, 0x00000000);
	
	// Setup link
	ctrl = igb_read(IGB_REG_CTRL);
	ctrl |= IGB_CTRL_SLU;
	igb_write(IGB_REG_CTRL, ctrl);
	
	// Interrupt throttling: Wait 1000 * 256ns = 256us between interrupts
	// TODO adjust this for performance optimization
	//igb_write(IGB_REG_ITR, 1000);
	//igb_write(IGB_REG_ITR, 0);
	// TODO this has changed for IGB devices

	// Allocate transmit data buffer
	uint64_t txBufferMemPhy;
	txBufferMem = heap_alloc_contiguous(TX_DESC_COUNT * TX_BUFFER_SIZE, VM_R | VM_W, &txBufferMemPhy);
	if(!txBufferMem)
		panic("Could not allocate igb transmit data buffer.");
	
	// Allocate and initialize transmit descriptor buffer
	uint64_t txDescriptorsPhy;
	txDescriptors = heap_alloc_contiguous(TX_DESC_COUNT * sizeof(tx_desc_t), VM_R | VM_W, &txDescriptorsPhy);
	if(!txDescriptors)
		panic("Could not allocate igb transmit descriptor buffer.");
	for(int i = 0; i < TX_DESC_COUNT; ++i)
	{
		// Initialize descriptor
		tx_desc_t *currDesc = &txDescriptors[i];
		currDesc->address = txBufferMemPhy + i * TX_BUFFER_SIZE;
		currDesc->length = 0;
		currDesc->status = 0;
		currDesc->cso = 0;
		currDesc->reserved = 0;
		currDesc->vlan = 0;
	}
	
	// Pass transmit descriptor buffer
	trace_printf("txDescriptorsPhy = %012x\n", txDescriptorsPhy);
	trace_printf("txBufferMemPhy = %012x\n", txBufferMemPhy);
	igb_write(IGB_REG_TDBAH, txDescriptorsPhy >> 32);
	igb_write(IGB_REG_TDBAL, txDescriptorsPhy & 0xFFFFFFFF);
	igb_write(IGB_REG_TDLEN, TX_DESC_COUNT * sizeof(tx_desc_t));
	igb_write(IGB_REG_TDH, 0);
	igb_write(IGB_REG_TDT, 0);
	txTail = 0;
	
	// Use only the first transmission queue
	uint32_t txdctl0 = igb_read(IGB_REG_TXDCTL0);
	txdctl0 |= 0x02000000; // Enable queue
	igb_write(IGB_REG_TXDCTL0, txdctl0);
	// Others are disabled by default
	
	// Allocate receive data buffer
	uint64_t rxBufferMemPhy;
	rxBufferMem = heap_alloc_contiguous(RX_DESC_COUNT * RX_BUFFER_SIZE, VM_R | VM_W, &rxBufferMemPhy);
	if(!rxBufferMem)
		panic("Could not allocate igb receive data buffer.");
	
	// Allocate and initialize receive descriptor buffer
	uint64_t rxDescriptorsPhy;
	rxDescriptors = heap_alloc_contiguous(RX_DESC_COUNT * sizeof(rx_desc_t), VM_R | VM_W, &rxDescriptorsPhy);
	if(!rxDescriptors)
		panic("Could not allocate igb receive descriptor buffer.");
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
	igb_write(IGB_REG_RDBAH, rxDescriptorsPhy >> 32);
	igb_write(IGB_REG_RDBAL, rxDescriptorsPhy & 0xFFFFFFFF);
	igb_write(IGB_REG_RDLEN, RX_DESC_COUNT * sizeof(rx_desc_t));
	igb_write(IGB_REG_RDH, 0);
	igb_write(IGB_REG_RDT, RX_DESC_COUNT - 1);
	rxTail = RX_DESC_COUNT - 1;
	
	// Use only the first receive queue
	uint32_t rxdctl0 = igb_read(IGB_REG_RXDCTL0);
	rxdctl0 |= 0x02000000; // Enable queue
	igb_write(IGB_REG_RXDCTL0, rxdctl0);
	// Others are disabled by default
	
	// Disable TCP/IP checksum offloading
	igb_write(IGB_REG_RXCSUM, 0x00000000);
	
	// Enable receiver
	uint32_t rctl = igb_read(IGB_REG_RCTL);
	rctl |= IGB_RCTL_EN; // EN (Receiver Enable)
	rctl |= IGB_RCTL_SBP; // SBP (Store Bad Packets)
	//rctl |= 0x00000020; // LPE (Long Packet Reception Enable)   -> MTU is set to 1522, thus we don't use this feature
	rctl |= IGB_RCTL_BAM; // BAM (Broadcast Accept Mode)
	rctl &= ~IGB_RCTL_SZ_256;
	rctl |= IGB_RCTL_SZ_2048; // BSIZE = 2048 (Receive Buffer Size)
	rctl |= IGB_RCTL_SECRC; // SECRC (Strip Ethernet CRC)
	rctl |= IGB_RCTL_MPE; // Promiscuous mode   -> for testing only!
	rctl |= IGB_RCTL_UPE; // Promiscuous mode   -> for testing only!
	igb_write(IGB_REG_RCTL, rctl);
	
	// Enable transmitter
	uint32_t tctl = igb_read(IGB_REG_TCTL);
	tctl |= IGB_TCTL_EN; // EN (Transmitter Enable)
	tctl |= IGB_TCTL_PSP; // PSP (Pad Short Packets)
	/*tctl &= ~IGB_TCTL_CT;
	tctl |= 0x000000F0; // 16 retries Collision Threshold*/
	/*tctl &= ~IGB_TCTL_COLD;
	tctl |= 0x0003F000; // 64-byte Collision Distance*/
	tctl |= IGB_TCTL_RTLC; // RTLC (Re-transmit on Late Collision)
	igb_write(IGB_REG_TCTL, tctl);
	
	// TODO extended status?
	

	
	
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

	// Enable receive interrupt
	igb_write(IGB_REG_IMS, 0x00000080);
	
	// Return hardware control
	// TODO The igb driver does not do this?!
	/*ctrlExt = igb_read(IGB_REG_CTRL_EXT);
	ctrlExt &= ~IGB_CTRL_EXT_DRV_LOAD;
	igb_write(IGB_REG_CTRL_EXT, ctrlExt);*/
	
	initialized = true;
	trace_printf("Network driver initialized.\n");
}

void igb_get_mac_address(uint8_t *macBuffer)
{
	// Copy MAC address
	for(int i = 0; i < 6; ++i)
		macBuffer[i] = macAddress[i];
}

static int debugRegs[] = {
	0x04000,	/* CRC Error Count - R/clr */
	0x04004,	/* Alignment Error Count - R/clr */
	0x04008,	/* Symbol Error Count - R/clr */
	0x0400C,	/* Receive Error Count - R/clr */
	0x04010,	/* Missed Packet Count - R/clr */
	0x04014,	/* Single Collision Count - R/clr */
	
	0x04018,	/* Excessive Collision Count - R/clr */
	0x0401C,	/* Multiple Collision Count - R/clr */
	0x04020,	/* Late Collision Count - R/clr */
	0x04028,	/* Collision Count - R/clr */
	0x04030,	/* Defer Count - R/clr */
	0x04034,	/* Tx-No CRS - R/clr */
	
	0x04038,	/* Sequence Error Count - R/clr */
	0x0403C,	/* Carrier Extension Error Count - R/clr */
	0x04040,	/* Receive Length Error Count - R/clr */
	0x04048,	/* XON Rx Count - R/clr */
	0x0404C,	/* XON Tx Count - R/clr */
	0x04050,	/* XOFF Rx Count - R/clr */
	
	0x04054,	/* XOFF Tx Count - R/clr */
	0x04058,	/* Flow Control Rx Unsupported Count- R/clr */
	0x0405C,	/* Packets Rx (64 bytes) - R/clr */
	0x04060,	/* Packets Rx (65-127 bytes) - R/clr */
	0x04064,	/* Packets Rx (128-255 bytes) - R/clr */
	0x04068,	/* Packets Rx (255-511 bytes) - R/clr */
	
	0x0406C,	/* Packets Rx (512-1023 bytes) - R/clr */
	0x04070,	/* Packets Rx (1024-1522 bytes) - R/clr */
	0x04074,	/* Good Packets Rx Count - R/clr */
	0x04078,	/* Broadcast Packets Rx Count - R/clr */
	0x0407C,	/* Multicast Packets Rx Count - R/clr */
	0x04080,	/* Good Packets Tx Count - R/clr */
	
	0x04088,	/* Good Octets Rx Count Low - R/clr */
	0x0408C,	/* Good Octets Rx Count High - R/clr */
	0x04090,	/* Good Octets Tx Count Low - R/clr */
	0x04094,	/* Good Octets Tx Count High - R/clr */
	0x040A0,	/* Rx No Buffers Count - R/clr */
	0x040A4,	/* Rx Undersize Count - R/clr */
	
	0x040A8,	/* Rx Fragment Count - R/clr */
	0x040AC,	/* Rx Oversize Count - R/clr */
	0x040B0,	/* Rx Jabber Count - R/clr */
	0x040B4,	/* Management Packets Rx Count - R/clr */
	0x040B8,	/* Management Packets Dropped Count - R/clr */
	0x040BC,	/* Management Packets Tx Count - R/clr */
	
	0x040C0,	/* Total Octets Rx Low - R/clr */
	0x040C4,	/* Total Octets Rx High - R/clr */
	0x040C8,	/* Total Octets Tx Low - R/clr */
	0x040CC,	/* Total Octets Tx High - R/clr */
	0x040D0,	/* Total Packets Rx - R/clr */
	0x040D4,	/* Total Packets Tx - R/clr */
	
	0x040D8,	/* Packets Tx (64 bytes) - R/clr */
	0x040DC,	/* Packets Tx (65-127 bytes) - R/clr */
	0x040E0,	/* Packets Tx (128-255 bytes) - R/clr */
	0x040E4,	/* Packets Tx (256-511 bytes) - R/clr */
	0x040E8,	/* Packets Tx (512-1023 bytes) - R/clr */
	0x040EC,	/* Packets Tx (1024-1522 Bytes) - R/clr */
	
	0x040F0,	/* Multicast Packets Tx Count - R/clr */
	0x040F4,	/* Broadcast Packets Tx Count - R/clr */
	0x040F8,	/* TCP Segmentation Context Tx - R/clr */
	0x040FC,	/* TCP Segmentation Context Tx Fail - R/clr */
	0x04100,	/* Interrupt Assertion Count */
	0x04104,	/* Interrupt Cause Rx Pkt Timer Expire Count */
	
	0x04108,	/* Interrupt Cause Rx Abs Timer Expire Count */
	0x0410C,	/* Interrupt Cause Tx Pkt Timer Expire Count */
	0x04110,	/* Interrupt Cause Tx Abs Timer Expire Count */
	0x04118,	/* Interrupt Cause Tx Queue Empty Count */
	0x0411C,	/* Interrupt Cause Tx Queue Min Thresh Count */
	0x04120,	/* Interrupt Cause Rx Desc Min Thresh Count */
	
	0x04124,	/* Interrupt Cause Receiver Overrun Count */
};

static void debug_regs()
{
	/*int cnt = sizeof(debugRegs) / sizeof(debugRegs[0]);
	for(int i = 0; i < cnt / 6; ++i)
	{
		trace_printf("%08x %08x %08x %08x %08x %08x\n",
			igb_read(debugRegs[6 * i + 0]),
			igb_read(debugRegs[6 * i + 1]),
			igb_read(debugRegs[6 * i + 2]),
			igb_read(debugRegs[6 * i + 3]),
			igb_read(debugRegs[6 * i + 4]),
			igb_read(debugRegs[6 * i + 5])
		);
	}*/
	
	trace_printf("-- Device status: %08x\n", igb_read(IGB_REG_STATUS));
	trace_printf("   CTRL = %08x\n", igb_read(IGB_REG_CTRL));
	trace_printf("   RCTL = %08x\n", igb_read(IGB_REG_RCTL));
	trace_printf("   RDBAH = %08x\n", igb_read(IGB_REG_RDBAH));
	trace_printf("   RDBAL = %08x\n", igb_read(IGB_REG_RDBAL));
	trace_printf("   RDH = %08x\n", igb_read(IGB_REG_RDH));
	trace_printf("   RDT = %08x\n", igb_read(IGB_REG_RDT));
	trace_printf("   RFCTL = %08x\n", igb_read(IGB_REG_RFCTL));
	trace_printf("   RXCSUM = %08x\n", igb_read(IGB_REG_RXCSUM));
	trace_printf("   MRQC = %08x\n", igb_read(IGB_REG_MRQC));
	trace_printf("   TCTL = %08x\n", igb_read(IGB_REG_TCTL));
	trace_printf("   TDBAH = %08x\n", igb_read(IGB_REG_TDBAH));
	trace_printf("   TDBAL = %08x\n", igb_read(IGB_REG_TDBAL));
	trace_printf("   TDH = %08x\n", igb_read(IGB_REG_TDH));
	trace_printf("   TDT = %08x\n", igb_read(IGB_REG_TDT));
	trace_printf("   TXDCTL0 = %08x\n", igb_read(IGB_REG_TXDCTL0));
	trace_printf("   TXDCTL1 = %08x\n", igb_read(IGB_REG_TXDCTL1));
	trace_printf("   TXDCTL2 = %08x\n", igb_read(IGB_REG_TXDCTL2));
	trace_printf("   TXDCTL3 = %08x\n", igb_read(IGB_REG_TXDCTL3));
	trace_printf("   RXDCTL0 = %08x\n", igb_read(IGB_REG_RXDCTL0));
	trace_printf("   RXDCTL1 = %08x\n", igb_read(IGB_REG_RXDCTL1));
	trace_printf("   RXDCTL2 = %08x\n", igb_read(IGB_REG_RXDCTL2));
	trace_printf("   RXDCTL3 = %08x\n", igb_read(IGB_REG_RXDCTL3));
	//trace_printf("   TARC1 = %08x\n", igb_read(IGB_REG_TARC1));
	trace_printf("   IMS = %08x\n", igb_read(IGB_REG_IMS));
	
	trace_printf("\n");
}

void igb_send(uint8_t *packet, int packetLength)
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
	desc->reserved = 0;
	desc->vlan = 0;
	
	// Update tail
	++txTail;
	if(txTail == TX_DESC_COUNT)
		txTail = 0;
	igb_write(IGB_REG_TDT, txTail);
	
	//trace_printf("Passing packet to device done.\n");
	
	//debug_regs();
}

// Processes one received packet.
// TODO use ITR (interrupt throttling register) to fire interrupts for multiple packets at once (less interrupts)
static void igb_receive()
{
	raw_spinlock_lock(&receiveLock);
	
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
			
			/*trace_printf("Received descriptor with length %d\n", packetLength);
			trace_printf("First bytes: ");
			int debugLen = (packetLength > 32 ? 32 : packetLength);
			for(int i = 0; i < debugLen; ++i)
				trace_printf("%02x ", packet[i]);
			trace_printf("...\n");*/
			
			// Errors?
			if(rxDescriptors[newRxTail].errors)
				trace_printf("Error byte in descriptor: %02x\n", rxDescriptors[newRxTail].errors);
			else if(packetLength > IGB_MTU)
				panic("Received packet size %d exceeds assumed MTU %d!\n", packetLength, IGB_MTU);
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
			igb_write(IGB_REG_RDT, rxTail);
		}
		else
			break; // No more received packets
	}
	
	raw_spinlock_unlock(&receiveLock);
}

int igb_next_received_packet(uint8_t *packetBuffer)
{
	// Any packet available?
	if(!receivedPacketsQueueStart)
		return 0;
	
	raw_spinlock_lock(&receiveLock);
	
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
	raw_spinlock_unlock(&receiveLock);
	return packetLength;
}

bool igb_handle_interrupt(cpu_state_t *state)
{
	// Ensure initialization is done
	if(!initialized)
		return false;
	
	// Read interrupt cause register
	uint32_t icr = igb_read(IGB_REG_ICR);
	if(!icr)
	{
		//trace_printf("Interrupt not caused by igb\n", icr);
		return false;
	}
	
	// Handle set interrupts
	//trace_printf("Intel igb interrupt! ICR: %08x\n", icr);
	if(icr & 0x00000080)
	{
		// Receive timer expired, handle received packets
		igb_receive();
	}
	else if(icr == 0x00000002) // TODO only for debugging - needed?
		igb_write(IGB_REG_ICR, 0x00000002);
	return true;
}