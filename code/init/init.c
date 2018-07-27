#include <trace/trace.h>
#include <init/cmdline.h>
#include <trace/stacktrace.h>
#include <mm/map.h>
#include <mm/phy32.h>
#include <mm/pmm.h>
#include <mm/vmm.h>
#include <mm/heap.h>
#include <mm/tlb.h>
#include <bus/isa.h>
#include <cpu/features.h>
#include <cpu/gdt.h>
#include <cpu/tss.h>
#include <cpu/idt.h>
#include <cpu/halt.h>
#include <intr/apic.h>
#include <intr/pic.h>
#include <intr/route.h>
#include <intr/fault.h>
#include <intr/nmi.h>
#include <panic/panic.h>
#include <smp/cpu.h>
#include <acpi/scan.h>
#include <smp/init.h>
#include <proc/sched.h>
#include <proc/syscall.h>
#include <proc/module.h>
#include <proc/idle.h>
#include <lock/intr.h>
#include <stdlib/string.h>
#include <stdbool.h>
#include <vbe/vbe.h>

static void print_banner(void)
{
  const int bannerWidth = 80;
  const char *bannerLine1 = "ITS Micro Kernel";
  const char *bannerLine2 = "based on Arc Operating System by Graham Edgecombe";
  
  int bannerLine1SpaceWidth = bannerWidth / 2 - strlen(bannerLine1) / 2;
  int bannerLine2SpaceWidth = bannerWidth / 2 - strlen(bannerLine2) / 2;

  char dashes[bannerWidth + 1];
  char bannerLine1Space[bannerLine1SpaceWidth + 1];
  char bannerLine2Space[bannerLine2SpaceWidth + 1];

  memset(dashes, '-', bannerWidth);
  dashes[bannerWidth] = '\0';
  
  memset(bannerLine1Space, ' ', bannerLine1SpaceWidth);
  bannerLine1Space[bannerLine1SpaceWidth] = '\0';
  
  memset(bannerLine2Space, ' ', bannerLine2SpaceWidth);
  bannerLine2Space[bannerLine2SpaceWidth] = '\0';

  trace_printf("%s\n", dashes);
  trace_printf("%s%s\n", bannerLine1Space, bannerLine1);
  trace_printf("%s%s\n", bannerLine2Space, bannerLine2);
  trace_printf("%s\n", dashes);
}

noreturn void init(uint32_t magic, multiboot_t *multiboot)
{
  /* convert physical 32-bit multiboot address to virtual address */
  multiboot = phy32_to_virt(multiboot);

  /* set up the BSP's percpu structure */
  cpu_bsp_init();

  /* parse command line arguments */
  cmdline_init(multiboot);
  
  // Initialize VBE for graphical rendering
  vbe_init(multiboot);

  /* initialize tracing */
  trace_init();

  /* print banner */
  print_banner();

  /* check the multiboot magic number */
  if (magic != MULTIBOOT_MAGIC)
    panic("invalid multiboot magic (expected %0#10x, got %0#10x)", MULTIBOOT_MAGIC, magic);

  /* set up stack traces */
  stacktrace_init(multiboot);

  /* set up the GDT, TSS, IDT and SYSCALL/RET */
  gdt_init();
  tss_init();
  idt_bsp_init();
  syscall_init();

  /* scan CPU features */
  cpu_features_init();

  /* map physical memory */
  trace_puts("Mapping physical memory...\n");
  list_t *map = mm_map_init(multiboot);

  /* set up the physical memory manager */
  trace_puts("Setting up the physical memory manager...\n");
  pmm_init(map);

  /* set up the virtual memory manager */
  trace_puts("Setting up the virtual memory manager...\n");
  vmm_init();

  /* set up the heap */
  trace_puts("Setting up the heap...\n");
  heap_init();

  /* init ISA bus */
  isa_init();

  /* uniprocessor fallback flag */
  bool up_fallback = false;

  /* search for ACPI tables */
  trace_puts("Scanning ACPI tables...\n");
  if (!acpi_scan())
  {
    /* fall back to non-SMP mode using the PIC */
    trace_puts("Falling back to single processor mode...\n");
    pic_init();
    up_fallback = true;
  }

  /* set up the local APIC on the BSP if we are in SMP mode */
  if (!up_fallback)
    apic_init();

  /* enable interrupts now the IDT and interrupt controllers are set up */
  intr_unlock();

  /* route IPIs */
  panic_init();
  fault_init();
  tlb_init();

  /* set up idle process, this must be done before we are in SMP mode */
  idle_init();

  /* set up symmetric multi-processing */
  if (!up_fallback)
  {
    trace_puts("Setting up SMP...\n");
    smp_init();
  }

  /* set up NMI routing, this must be done when we are in SMP mode */
  nmi_init();

  /* set up the scheduler, also needs SMP mode */
  sched_init();

  /* set up modules */
  module_init(multiboot);

  /* halt forever - the scheduler will take over from here */
  halt_forever();
}
