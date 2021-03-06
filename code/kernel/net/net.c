/*
Ethernet driver interface.
*/

#include <net/net.h>
#include <net/e1000.h>
#include <net/e1000e.h>
#include <net/igb.h>
#include <trace/trace.h>
#include <mm/mmio.h>
#include <panic/panic.h>
#include <stdlib/stdlib.h>
#include <stdlib/string.h>
#include <mm/heap.h>
#include <cpu/pause.h>

// Pointer to the network device initialization function.
static void (*netdev_init)(pci_cfgspace_header_0_t *deviceCfgSpaceHeader) = 0;

// Pointer to the network device MAC address retrieval function.
static void (*netdev_get_mac_address)(uint8_t *macBuffer) = 0;

// Pointer to the network device send function.
static void (*netdev_send)(uint8_t *packet, int packetLength) = 0;

// Pointer to the network device receive function.
static int (*netdev_next_received_packet)(uint8_t *packetBuffer) = 0;

// Pointer to the network device interrupt handling function.
static bool (*netdev_handle_interrupt)(cpu_state_t *state) = 0;

// Intel device IDs.
static uint16_t deviceIdsE1000[] = { 0x1000, 0x1001, 0x1002, 0x1004, 0x1008, 0x1009, 0x100a, 0x100c,
                                          0x100d, 0x100e, 0x100f, 0x1010, 0x1011, 0x1012, 0x1013, 0x1014,
										  0x1015, 0x1016, 0x1017, 0x1018, 0x1019, 0x101a, 0x101d, 0x101e,
										  0x1026, 0x1027, 0x1028 };
static uint16_t deviceIdsE1000e[] = { 0x15b8, 0x15d6, 0x15d8, 0x10d3, 0x1570, 0x15a3 };
static uint16_t deviceIdsIgb[] = { 0x1539 };

// Returns whether the given device ID is a member of the given device list.
static bool net_is_device(uint16_t deviceId, uint16_t *deviceIdList, int deviceIdListCount)
{
	for(int i = 0; i < deviceIdListCount; ++i)
		if(deviceId == deviceIdList[i])
			return true;
	return false;
}

void net_init(pci_cfgspace_header_0_t *deviceCfgSpaceHeader)
{
	// Pick correct driver depending on PCI device ID
	uint16_t vendorId = deviceCfgSpaceHeader->commonHeader.vendorId;
	uint16_t deviceId = deviceCfgSpaceHeader->commonHeader.deviceId;
	if(vendorId == 0x8086)
	{
		// Search device ID arrays
		bool found = false;
		if(net_is_device(deviceId, deviceIdsE1000, sizeof(deviceIdsE1000) / sizeof(deviceIdsE1000[0])))
		{
			trace_printf("Recognized e1000 network device, loading driver...\n");
			netdev_init = &e1000_init;
			netdev_get_mac_address = &e1000_get_mac_address;
			netdev_send = &e1000_send;
			netdev_next_received_packet = &e1000_next_received_packet;
			netdev_handle_interrupt = &e1000_handle_interrupt;
			found = true;
		}
		else if(net_is_device(deviceId, deviceIdsE1000e, sizeof(deviceIdsE1000e) / sizeof(deviceIdsE1000e[0])))
		{
			trace_printf("Recognized e1000e network device, loading driver...\n");
			netdev_init = &e1000e_init;
			netdev_get_mac_address = &e1000e_get_mac_address;
			netdev_send = &e1000e_send;
			netdev_next_received_packet = &e1000e_next_received_packet;
			netdev_handle_interrupt = &e1000e_handle_interrupt;
			found = true;
		}
		else if(net_is_device(deviceId, deviceIdsIgb, sizeof(deviceIdsIgb) / sizeof(deviceIdsIgb[0])))
		{
			trace_printf("Recognized igb network device, loading driver...\n");
			netdev_init = &igb_init;
			netdev_get_mac_address = &igb_get_mac_address;
			netdev_send = &igb_send;
			netdev_next_received_packet = &igb_next_received_packet;
			netdev_handle_interrupt = &igb_handle_interrupt;
			found = true;
		}
		
		// If device was found, initialize it
		if(found)
			netdev_init(deviceCfgSpaceHeader);
		else
			trace_printf("Unsupported network device\n");
	}
	else
		trace_printf("No network device detected.\n");
}

void net_get_mac_address(uint8_t *macBuffer)
{
	// Call device function, if it exists
	if(!netdev_get_mac_address)
		panic("Call of net_get_mac_address() without recognized network device\n");
	netdev_get_mac_address(macBuffer);
}

void net_send(uint8_t *packet, int packetLength)
{
	// Call device function, if it exists
	if(!netdev_send)
		panic("Call of net_send() without recognized network device\n");
	netdev_send(packet, packetLength);
}

int net_next_received_packet(uint8_t *packetBuffer)
{
	// Call device function, if it exists
	if(!netdev_next_received_packet)
		panic("Call of net_next_received_packet() without recognized network device\n");
	return netdev_next_received_packet(packetBuffer);
}

bool net_handle_interrupt(cpu_state_t *state)
{
	// Call device function, if it exists
	if(netdev_handle_interrupt)
		return netdev_handle_interrupt(state);
	return false;
}