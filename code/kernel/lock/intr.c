
#include <lock/intr.h>
#include <cpu/intr.h>
#include <smp/cpu.h>
#include <stdlib/assert.h>

void intr_lock(void)
{
  cpu_t *cpu = cpu_get();
  assert(cpu->intr_mask_count != UINT64_MAX);
  if (cpu->intr_mask_count++ == 0)
    intr_disable();
}

void intr_unlock(void)
{
  cpu_t *cpu = cpu_get();
  assert(cpu->intr_mask_count != 0);
  if (--cpu->intr_mask_count == 0)
    intr_enable();
}
