
#include <mm/validate.h>
#include <mm/common.h>
#include <stdint.h>

bool valid_buffer(const void *ptr, size_t len)
{
  uintptr_t addr_start = (uintptr_t) ptr;
  uintptr_t addr_end = addr_start + len;
  return addr_start <= VM_USER_END && addr_end <= VM_USER_END && addr_start <= addr_end;
}

bool valid_string(const char *str)
{
  for (;;)
  {
    if ((uintptr_t) str > VM_USER_END)
      return false;

    if (*str++ == 0)
      return true;
  }
}
