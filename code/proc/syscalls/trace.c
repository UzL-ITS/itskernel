
#include <proc/syscalls.h>
#include <mm/validate.h>
#include <trace/trace.h>

int64_t sys_trace(const char *message)
{
  if (!valid_string(message))
    return -1; // TODO: return some meaningful err number

  trace_puts(message);
  return 0;
}
