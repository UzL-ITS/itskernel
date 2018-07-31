
#ifndef _ACPI_COMMON_H
#define _ACPI_COMMON_H

#include <stdbool.h>
#include <stdint.h>

#define OEM_ID_LEN 6

typedef struct
{
  uint32_t signature;
  uint32_t len;
  uint8_t  revision;
  uint8_t  checksum;
  char     oem_id[OEM_ID_LEN];
  uint64_t oem_table_id;
  uint32_t oem_revision;
  uint32_t creator_id;
  uint32_t creator_revision;
} __attribute__((__packed__)) acpi_header_t;

bool acpi_table_valid(acpi_header_t *table);

#endif
