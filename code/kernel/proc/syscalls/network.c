
#include <proc/syscalls.h>
#include <smp/cpu.h>
#include <net/intel8254x.h>

void sys_get_network_mac_address(uint8_t *macBuffer)
{
	intel8254x_get_mac_address(macBuffer);
}

int sys_receive_network_packet(uint8_t *packetBuffer)
{
	return intel8254x_next_received_packet(packetBuffer);
}

void sys_send_network_packet(uint8_t *packet, int packetLength)
{
	intel8254x_send(packet, packetLength);
}