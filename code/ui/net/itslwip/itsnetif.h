#pragma once

/*
ITS kernel LWIP network driver interface.
*/

/* INCLUDES */

#include <stdint.h>
#include "lwip/netif.h"


/* CONSTANTS */

// Maximum ethernet packet size for an MTU of 1500.
#define ETHERNET_PACKET_SIZE 1522


/* DECLARATIONS */

// Handles a received ethernet packet.
void itsnetif_input(struct netif *netif, uint8_t *packet, int packetLength);

// Initializes the network driver interface.
err_t itsnetif_init(struct netif *netif);