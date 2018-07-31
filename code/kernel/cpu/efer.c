
#include <cpu/efer.h>
#include <cpu/msr.h>

uint64_t efer_read(void)
{
  return msr_read(MSR_EFER);
}

void efer_write(uint64_t efer)
{
  msr_write(MSR_EFER, efer);
}
