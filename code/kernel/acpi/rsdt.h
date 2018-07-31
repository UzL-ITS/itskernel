
#ifndef _ACPI_RSDT_H
#define _ACPI_RSDT_H

#include <stdint.h>
#include <acpi/common.h>

#define RSDT_SIGNATURE 0x54445352 /* 'RSDT' */

typedef struct
{
  acpi_header_t header;
  uint32_t entries[1];
} __attribute__((__packed__)) rsdt_t;

#endif
