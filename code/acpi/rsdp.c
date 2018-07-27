
#include <acpi/rsdp.h>
#include <mm/phy32.h>
#include <trace/bda.h>
#include <stdbool.h>
#include <stddef.h>

static bool rsdp_valid(rsdp_t *rsdp)
{
  if (rsdp->signature != RSDP_SIGNATURE)
    return false;

  uint8_t sum = 0;
  uint8_t *ptr_start = (uint8_t *) rsdp;
  uint8_t *ptr_end = ptr_start + offsetof(rsdp_t, len);

  for (uint8_t *ptr = ptr_start; ptr < ptr_end; ptr++)
    sum += *ptr;

  if (sum != 0)
    return false;

  if (rsdp->revision < 2)
    return true;

  sum = 0;
  ptr_end = ptr_start + rsdp->len;

  for (uint8_t *ptr = ptr_start; ptr < ptr_end; ptr++)
    sum += *ptr;

  return sum == 0;
}

static rsdp_t *rsdp_scan_range(uintptr_t start, uintptr_t end)
{
  start = aphy32_to_virt(start);
  end = aphy32_to_virt(end);

  for (uintptr_t ptr = start; ptr < end; ptr += RSDP_ALIGN)
  {
    rsdp_t *rsdp = (rsdp_t *) ptr;
    if (rsdp_valid(rsdp))
      return rsdp;
  }

  return 0;
}

rsdp_t *rsdp_scan(void)
{
  /* find the address of the EBDA */
  uintptr_t ebda = bda_reads(BDA_EBDA) << 4;

  /* search the first kilobyte of the EBDA */
  rsdp_t *rsdp = rsdp_scan_range(ebda, ebda + 1024);
  if (rsdp)
    return rsdp;

  /* search the shadow BIOS ROM */
  rsdp = rsdp_scan_range(0xE0000, 0x100000);
  if (rsdp)
    return rsdp;

  return 0;
}
