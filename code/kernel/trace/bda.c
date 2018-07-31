
#include <trace/bda.h>
#include <mm/phy32.h>

#define BDA_OFFSET 0x400

uint8_t bda_read(uint8_t off)
{
  return *((uint8_t *) aphy32_to_virt(BDA_OFFSET + off));
}

uint16_t bda_reads(uint8_t off)
{
  return *((uint16_t *) aphy32_to_virt(BDA_OFFSET + off));
}
