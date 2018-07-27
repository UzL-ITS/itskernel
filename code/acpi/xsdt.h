
#ifndef _ACPI_XSDT_H
#define _ACPI_XSDT_H

#include <stdint.h>
#include <acpi/common.h>

#define XSDT_SIGNATURE 0x54445358 /* 'XSDT' */

typedef struct
{
  acpi_header_t header;
  uint64_t entries[1];
} __attribute__((__packed__)) xsdt_t;

#endif
