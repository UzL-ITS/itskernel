
#include <acpi/scan.h>
#include <acpi/rsdp.h>
#include <acpi/rsdt.h>
#include <acpi/xsdt.h>
#include <acpi/madt.h>
#include <mm/mmio.h>
#include <init/cmdline.h>
#include <panic/panic.h>
#include <trace/trace.h>
#include <stddef.h>
#include <stdlib/string.h>

static acpi_header_t *acpi_map(uintptr_t addr)
{
  acpi_header_t *table = mmio_map(addr, sizeof(*table), VM_R);
  if (!table)
    return 0;

  uint32_t len = table->len;
  mmio_unmap(table, sizeof(*table));
  return mmio_map(addr, len, VM_R);
}

static void acpi_unmap(acpi_header_t *table)
{
  mmio_unmap(table, table->len);
}

static void acpi_scan_table(uintptr_t addr)
{
  /* map the table into virtual memory */
  acpi_header_t *table = acpi_map(addr); 
  if (!table)
    panic("couldn't map table");

  /* check to see if the table has a valid checksum */
  if (!acpi_table_valid(table))
  {
    char sig[5];
    sig[0] = table->signature & 0xFF;
    sig[1] = (table->signature >> 8) & 0xFF;
    sig[2] = (table->signature >> 16) & 0xFF;
    sig[3] = (table->signature >> 24) & 0xFF;
    sig[4] = 0;

    panic("invalid checksum in %s", sig);
  }

  /* scan the MADT */
  if (table->signature == MADT_SIGNATURE)
    madt_scan((madt_t *) table);

  /* we're done with it, unmap it */
  acpi_unmap(table);
}

bool acpi_scan(void)
{
  /* check if ACPI is enabled */
  const char *acpi = cmdline_get("acpi");
  if (acpi && strcmp(acpi, "off") == 0)
  {
    trace_puts(" => ACPI disabled by kernel command line\n");
    return false;
  }

  /* try to find the RSDP structure */
  rsdp_t *rsdp = rsdp_scan();
  if (!rsdp)
  {
    trace_puts(" => ACPI not supported\n");
    return false;
  }

  /* check the ACPI revision */
  if (rsdp->revision >= 2)
  {
    /* map the XSDT into virtual memory */
    xsdt_t *xsdt = (xsdt_t *) acpi_map(rsdp->xsdt_addr);
    if (!xsdt)
      panic("couldn't map XSDT");

    /* check the validity of the XSDT */
    if (!acpi_table_valid((acpi_header_t *) xsdt))
      panic("invalid checksum in XSDT");

    /* calculate the number of entries */
    size_t len = (xsdt->header.len - sizeof(xsdt->header)) / sizeof(xsdt->entries[0]);

    /* iterate through all of the tables */
    for (size_t i = 0; i < len; i++)
      acpi_scan_table((uintptr_t) xsdt->entries[i]);

    /* unmap XSDT */
    acpi_unmap((acpi_header_t *) xsdt);
  }
  else
  {
    /* map the RSDT into virtual memory */
    rsdt_t *rsdt = (rsdt_t *) acpi_map(rsdp->rsdt_addr);
    if (!rsdt)
      panic("couldn't map RSDT");

    /* check the validity of the RSDT */
    if (!acpi_table_valid((acpi_header_t *) rsdt))
      panic("invalid checksum in RSDT");

    /* calculate the number of entries */
    size_t len = (rsdt->header.len - sizeof(rsdt->header)) / sizeof(rsdt->entries[0]);

    /* iterate through all of the tables */
    for (size_t i = 0; i < len; i++)
      acpi_scan_table((uintptr_t) rsdt->entries[i]);

    /* unmap RSDT */
    acpi_unmap((acpi_header_t *) rsdt);
  }

  return true;
}
