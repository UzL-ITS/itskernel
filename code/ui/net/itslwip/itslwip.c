/*
ITS kernel LWIP port main file.
*/

/* INCLUDES */

#include "itslwip.h"
#include <stdbool.h>
#include <internal/syscall/syscalls.h>
#include "itsnetif.h"

#include "lwip/inet.h"
#include "lwip/tcp.h"
#include "lwip/netif.h"
#include "lwip/init.h"
#include "lwip/stats.h"
#include "netif/etharp.h"
#include "lwip/timeouts.h"


/* VARIABLES */

// LWIP start time in milliseconds.
static uint64_t startTime;

// LWIP network interface.
static struct netif lwip_netif;

// Addresses.
// TODO use DHCP?
static ip4_addr_t address;
static ip4_addr_t netmask;
static ip4_addr_t gatewayAddress;


/* LWIP SYSTEM FUNCTIONS */

void sys_init()
{
	// Remember LWIP start time
	startTime = sys_get_elapsed_milliseconds();
}

uint32_t sys_now()
{
	// Calculate LWIP relative time
	return (uint32_t)(sys_get_elapsed_milliseconds() - startTime);
}


/* FUNCTIONS */

static void handle_tcp_error(void *arg, err_t err)
{
	printf("TCP error: %d\n", err);
}

static err_t handle_tcp_connected(void *arg, struct tcp_pcb *tcpHandle, err_t err)
{
	printf("TCP connected with error %d\n", err);
	
	return ERR_OK;
}

static err_t handle_tcp_sent(void *arg, struct tcp_pcb *tcpHandle, u16_t len)
{
	printf("TCP sent %d bytes\n", len);
	
	//printf("TCP close(): %d\n", tcp_close(tcpHandle));
	
	return ERR_OK;
}

static err_t handle_tcp_receive(void *arg, struct tcp_pcb *tcpHandle, struct pbuf *p, err_t err)
{
	printf("TCP received with error %d:\n", err);
	
	char packet[10000] = { 0 };
	int packetSize = 0;
	for(struct pbuf *q = p; q != 0; q = q->next)
	{
		// Valid pbuf?
		if(q->payload && q->len)
		{
			// Copy contents
			memcpy(packet + packetSize, q->payload, q->len);
			packetSize += q->len;
		}
	}
	
	printf("Packet size: %d", packetSize);
	for(int i = 0; i < packetSize; ++i)
	{
		if(i % 16 == 0)
			printf("\n");
		printf("%02x ", packet[i]);
	}
	printf("\n");
	
	return ERR_OK;
}

static void test()
{
	ip4_addr_t targetAddr;
	inet_aton("192.168.20.1", &targetAddr);
	
	struct tcp_pcb *tcpHandle = tcp_new();
	tcp_err(tcpHandle, &handle_tcp_error);
	tcp_recv(tcpHandle, &handle_tcp_receive);
	
	printf("TCP connect(): %d\n", tcp_connect(tcpHandle, &targetAddr, 4242, &handle_tcp_connected));
	
	char data[] = "a b c d e\n\n";
	tcp_sent(tcpHandle, &handle_tcp_sent);
	printf("TCP write(): %d\n", tcp_write(tcpHandle, data, sizeof(data), 0));
	
}

void itslwip_run(void *args)
{
	// Initialize LWIP
	lwip_init();
	
	// Initialize LWIP network interface
	inet_aton("192.168.20.10", &address);
	inet_aton("255.255.255.0", &netmask);
	inet_aton("192.168.20.1", &gatewayAddress);
	if(!netif_add(&lwip_netif, &address, &netmask, &gatewayAddress, 0, &itsnetif_init, &ethernet_input))
	{
		printf("Error in netif_add()\n");
		return;
	}
	netif_set_default(&lwip_netif);
	netif_set_up(&lwip_netif);
	
	// Poll for new packets and send ticks to LWIP
	uint8_t packetBuffer[ETHERNET_PACKET_SIZE];
	bool testDone = false;
	while(true)
	{
		// Packet received?
		int packetLength = sys_receive_network_packet(packetBuffer);
		if(packetLength)
		{
			
			printf("Received packet of length %d", packetLength);
			for(int i = 0; i < packetLength; ++i)
			{
				if(i % 16 == 0)
					printf("\n    ");
				printf("%02x ", packetBuffer[i]);
			}
			printf("\n");
		
			itsnetif_input(&lwip_netif, packetBuffer, packetLength);
		}
		
		// Update timers
		sys_check_timeouts();
		
		if(!testDone)
		{
			test();
			testDone = true;
		}
	}
}