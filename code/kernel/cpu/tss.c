
#include <cpu/tss.h>
#include <smp/cpu.h>
#include <stdlib/string.h>

void tss_init(void)
{
  /* find this CPU's TSS */
  cpu_t *cpu = cpu_get();
  tss_t *tss = &cpu->tss;

  /* reset all the fields */
  memset(tss, 0, sizeof(*tss));
  tss->iomap_base = sizeof(*tss);

  /* install it using the LTR instruction */
  tss_install(SLTR_TSS);
}

void tss_set_rsp0(uint64_t rsp0)
{
  /* find this CPU's TSS */
  cpu_t *cpu = cpu_get();
  tss_t *tss = &cpu->tss;

  /* set the stack pointer for this CPU */
  tss->rsp0 = rsp0;
}
