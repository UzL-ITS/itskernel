
#include <cpu/port.h>

uint8_t inb_p(uint16_t port)
{
  uint8_t value = inb(port);
  iowait();
  return value;
}

uint16_t inw_p(uint16_t port)
{
  uint16_t value = inw(port);
  iowait();
  return value;
}

uint32_t inl_p(uint16_t port)
{
  uint32_t value = inl(port);
  iowait();
  return value;
}

void outb_p(uint16_t port, uint8_t value)
{
  outb(port, value);
  iowait();
}

void outw_p(uint16_t port, uint16_t value)
{
  outw(port, value);
  iowait();
}

void outl_p(uint16_t port, uint32_t value)
{
  outl(port, value);
  iowait();
}
