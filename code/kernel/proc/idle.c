
#include <proc/idle.h>
#include <proc/proc.h>
#include <cpu/halt.h>
#include <smp/cpu.h>
#include <cpu/gdt.h>
#include <cpu/cr.h>
#include <cpu/flags.h>
#include <util/container.h>
#include <panic/panic.h>

static proc_t *idle_proc;

void idle_init(void)
{
  /*
   * Keep the old PML4 table pointer and restore it later.
   *
   * This is a hacky solution to a problem where the SMP boot code maps a page
   * in lower memory containing the bootstrap code in the address space of the
   * idle process. The APs start up using the bootstrap address space, which
   * does not contain this mapping, causing a page fault.
   */
  uintptr_t old_pml4_table = cr3_read();

  idle_proc = proc_create();
  if (!idle_proc)
    panic("couldn't create idle process");

  proc_switch(idle_proc);

  list_for_each(&cpu_list, node)
  {
    cpu_t *cpu = container_of(node, cpu_t, node);
    thread_t *thread = thread_create(idle_proc, THREAD_KERNEL);
    if (!thread)
      panic("couldn't create idle thread");

    thread->rip = (uint64_t) &halt_forever;
    proc_thread_add(idle_proc, thread);

    cpu->idle_thread = thread;
  }

  cr3_write(old_pml4_table);
}
