
#include <trace/e9.h>
#include <cpu/port.h>
#include <panic/panic.h>

void e9_init(void)
{
  if (inb_p(0xE9) != 0xE9)
    panic("bochs e9 hack not enabled");
}

void e9_putch(char c)
{
  outb_p(0xE9, c);
}

void e9_puts(const char *str)
{
  for (char c; (c = *str++);)
    e9_putch(c);
}
