
#include <proc/syscalls.h>
#include <smp/cpu.h>
#include <net/net.h>

void sys_get_network_mac_address(uint8_t *macBuffer)
{
	net_get_mac_address(macBuffer);
}

int sys_receive_network_packet(uint8_t *packetBuffer)
{
	return net_next_received_packet(packetBuffer);
}

void sys_send_network_packet(uint8_t *packet, int packetLength)
{
	net_send(packet, packetLength);
}