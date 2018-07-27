
#ifndef _ACPI_RSDP_H
#define _ACPI_RSDP_H

#include <stdint.h>
#include <acpi/common.h>

#define RSDP_SIGNATURE 0x2052545020445352 /* 'RSD PTR ' */
#define RSDP_ALIGN     16

typedef struct
{
  /* original rsdp structure */
  uint64_t signature;
  uint8_t  checksum;
  char     oem_id[OEM_ID_LEN];
  uint8_t  revision;
  uint32_t rsdt_addr;

  /* extended fields - present if revision >= 2 */
  uint32_t len;
  uint64_t xsdt_addr;
  uint8_t  ext_checksum;
  uint8_t  reserved[3];
} __attribute__((__packed__)) rsdp_t;

rsdp_t *rsdp_scan(void);

#endif
