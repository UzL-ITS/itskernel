#pragma once

#include <stdint.h>
#include <pci/pci.h>

// Initializes the ethernet device using the given PCI configuration space header.
void intel8254x_init(pci_cfgspace_header_0_t *deviceCfgSpaceHeader);