
#include <smp/cpu.h>
#include <cpu/msr.h>
#include <stdlib/stdlib.h>
#include <stdlib/string.h>

list_t cpu_list = LIST_EMPTY;

static cpu_t cpu_bsp;

static int nextCoreId = 0;

void cpu_bsp_init(void)
{
  memset(&cpu_bsp, 0, sizeof(cpu_bsp));
  cpu_bsp.self = &cpu_bsp;
  cpu_bsp.bsp = true;
  cpu_bsp.intr_mask_count = 1; // as when this is called, interrupts are masked -> init.c calls intr_unlock() eventually
  cpu_bsp.proc = 0;
  cpu_bsp.thread = 0;
  cpu_bsp.coreId = nextCoreId++;
  cpu_bsp.tlbOperationQueueLength = 0;
  msr_write(MSR_GS_BASE, (uint64_t) &cpu_bsp);
  msr_write(MSR_GS_KERNEL_BASE, (uint64_t) &cpu_bsp);

  list_add_tail(&cpu_list, &cpu_bsp.node);
}

bool cpu_ap_init(cpu_lapic_id_t lapic_id, cpu_acpi_id_t acpi_id)
{
  cpu_t *cpu = malloc(sizeof(*cpu));
  if (!cpu)
    return false;

  memclr(cpu, sizeof(*cpu));

  cpu->self = cpu;
  cpu->lapic_id = lapic_id;
  cpu->acpi_id = acpi_id;
  cpu->intr_mask_count = 1; // as when this is called, interrupts are masked
  cpu->proc = 0;
  cpu->thread = 0;
  cpu->coreId = nextCoreId++;
  cpu->tlbOperationQueueLength = 0;

  list_add_tail(&cpu_list, &cpu->node);
  return true;
}

void cpu_ap_install(cpu_t *cpu)
{
  msr_write(MSR_GS_BASE, (uint64_t) cpu);
  msr_write(MSR_GS_KERNEL_BASE, (uint64_t) cpu);
}

cpu_t *cpu_get_bsp()
{
	return &cpu_bsp;
}