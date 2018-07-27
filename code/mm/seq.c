
#include <mm/seq.h>
#include <trace/trace.h>
#include <stdint.h>

#define SEQ_SIZE 8192

static uint8_t seq_pool[SEQ_SIZE];
static size_t seq_pos = 0;

void *seq_alloc(size_t len)
{
  if ((seq_pos + len) > SEQ_SIZE)
    return 0;

  void *ptr = &seq_pool[seq_pos];
  seq_pos += len;
  return ptr;
}

void seq_trace(void)
{
  trace_printf("Tracing sequential allocator... %d/%d bytes used.\n", seq_pos, SEQ_SIZE);
}
