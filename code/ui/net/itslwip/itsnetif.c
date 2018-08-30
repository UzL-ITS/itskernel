/*
ITS kernel LWIP network driver interface.
Based on src/itsnetif.c.
*/

/* INCLUDES */

#include "itsnetif.h"
#include <memory.h>
#include <internal/syscall/syscalls.h>
#include <io.h>

#include "lwip/opt.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/stats.h"
#include "lwip/snmp.h"
#include "lwip/etharp.h"


/* TYPES */

// Struct for private data needed to operate this ethernet network driver.
typedef struct
{
	int dummy;
} itsnetif_t;


/* FUNCTIONS */

// Sends the given packet.
static err_t itsnetif_output(struct netif *netif, struct pbuf *p)
{
	// Get internal data
	//itsnetif_t *itsnetif = netif->state;
	
	// Buffer for merged packet
	char packet[ETHERNET_PACKET_SIZE] = { 0 };
	int packetSize = 0;

#if ETH_PAD_SIZE
	// Drop padding word
	pbuf_header(p, -ETH_PAD_SIZE);
#endif
	
	// Run through output pbuf chain and merge these into the packet buffer
	for(struct pbuf *q = p; q != 0; q = q->next)
	{
		// Valid pbuf?
		if(q->payload && q->len)
		{
			// Copy contents
			memcpy(packet + packetSize, q->payload, q->len);
			packetSize += q->len;
			
			// Error check
			if(packetSize > ETHERNET_PACKET_SIZE)
			{
				printf("Error: Buffer overflow when merging output ethernet packet.\n");
				return ERR_BUF;
			}
		}
	}

	printf("Sending packet of length %d", packetSize);
	for(int i = 0; i < packetSize; ++i)
	{
		if(i % 16 == 0)
			printf("\n    ");
		printf("%02x ", (uint8_t)packet[i]);
	}
	printf("\n");
	
	// Send packet
	sys_send_network_packet((uint8_t *)packet, packetSize);

	// Update statistics
	MIB2_STATS_NETIF_ADD(netif, ifoutoctets, p->tot_len);
	if(((u8_t *)p->payload)[0] & 1)
	{
		// Broadcast/Multicast
		MIB2_STATS_NETIF_INC(netif, ifoutnucastpkts);
	}
	else
	{
		// Unicast
		MIB2_STATS_NETIF_INC(netif, ifoutucastpkts);
	}
	LINK_STATS_INC(link.xmit);

#if ETH_PAD_SIZE
	// Reclaim padding word
	pbuf_header(p, ETH_PAD_SIZE);
#endif

	// Done
	return ERR_OK;
}

void itsnetif_input(struct netif *netif, uint8_t *packet, int packetLength)
{
	// Get internal data
	//itsnetif_t *itsnetif = netif->state;

#if ETH_PAD_SIZE
	// Allow room for ethernet padding
	packetLength += ETH_PAD_SIZE;
#endif

	// Allocate pbuf chain from pool and copy packet data
	struct pbuf *p = pbuf_alloc(PBUF_RAW, packetLength, PBUF_POOL);
	if(p != 0)
	{
#if ETH_PAD_SIZE
		// Drop padding word
		pbuf_header(p, -ETH_PAD_SIZE);
#endif

		// Iterate over pbuf chain until whole packet is copied
		int packetPos = 0;
		for(struct pbuf *q = p; q != 0; q = q->next)
		{
			// Copy packet part
			memcpy(p->payload, packet + packetPos, q->len);
			packetPos += q->len;
		}

		// Update statistics
		MIB2_STATS_NETIF_ADD(netif, ifinoctets, p->tot_len);
		if(((u8_t *)p->payload)[0] & 1)
		{
			// Broadcast/Multicast
			MIB2_STATS_NETIF_INC(netif, ifinnucastpkts);
		}
		else
		{
			// Unicast
			MIB2_STATS_NETIF_INC(netif, ifinucastpkts);
		}
		LINK_STATS_INC(link.recv);
		
#if ETH_PAD_SIZE
		// Reclaim padding word
		pbuf_header(p, ETH_PAD_SIZE);
#endif

		// Pass packet to LWIP ethernet input for further processing
		if(netif->input(p, netif) != ERR_OK)
		{
			printf("Error passing input packet to LWIP\n");
			pbuf_free(p);
		}
	}
	else
	{
		// Update statistics
		printf("pbuf allocation error\n");
		LINK_STATS_INC(link.memerr);
		LINK_STATS_INC(link.drop);
		MIB2_STATS_NETIF_INC(netif, ifindiscards);
	}
}

err_t itsnetif_init(struct netif *netif)
{
	// Parameter check
	LWIP_ASSERT("netif != NULL", (netif != 0));

	// Allocate memory for internal data
	itsnetif_t *itsnetif = mem_malloc(sizeof(itsnetif_t));
	if(!itsnetif)
		return ERR_MEM;
	netif->state = itsnetif;

#if LWIP_NETIF_HOSTNAME
	// Set interface host name
	netif->hostname = "itskernel";
#endif

	// Initialize SNMP variables, pass link speed in bits per second
	// TODO retrieve this from device driver? Needed at all?
	MIB2_INIT_NETIF(netif, snmp_ifType_ethernet_csmacd, 100000000);

	// Set interface name (arbitrary)
	netif->name[0] = 'i';
	netif->name[1] = 'i';

	// Pass function pointers
	netif->output = etharp_output;
#if LWIP_IPV6
	netif->output_ip6 = ethip6_output;
#endif
	netif->linkoutput = itsnetif_output;
	
	// Set MAC hardware address
	netif->hwaddr_len = ETHARP_HWADDR_LEN;
	sys_get_network_mac_address((uint8_t *)&netif->hwaddr);
	printf("MAC: %02x:%02x:%02x:%02x:%02x:%02x\n", netif->hwaddr[0], netif->hwaddr[1], netif->hwaddr[2], netif->hwaddr[3], netif->hwaddr[4], netif->hwaddr[5]);

	// Set maximum transfer unit
	// TODO configure driver correctly
	netif->mtu = 1500;

	// Assign device capabilities
	/* TODO ??? don't set NETIF_FLAG_ETHARP if this device is not an ethernet one */
	netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;
	
	// TODO any other device init?

	// Done
	return ERR_OK;
}
