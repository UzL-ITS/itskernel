#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <pci/pci.h>
#include <cpu/state.h>

void debug();

// Initializes the ethernet device using the given PCI configuration space header.
void net_init(pci_cfgspace_header_0_t *deviceCfgSpaceHeader);

// Writes the device MAC address into the given buffer of size 6.
void net_get_mac_address(uint8_t *macBuffer);

// Sends the given packet.
void net_send(uint8_t *packet, int packetLength);

// Returns the contents and the length of the oldest unprocessed packet.
// If there is no packet present, 0 is returned.
int net_next_received_packet(uint8_t *packetBuffer);

// Checks whether this device caused a PCI interrupt and handles it.
// If the interrupt was caused by another device, false is returned.
bool net_handle_interrupt(cpu_state_t *state);