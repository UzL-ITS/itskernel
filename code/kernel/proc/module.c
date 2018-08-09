
#include <proc/module.h>
#include <cpu/cr.h>
#include <lock/intr.h>
#include <proc/proc.h>
#include <proc/elf64.h>
#include <mm/phy32.h>
#include <panic/panic.h>
#include <stddef.h>
#include <trace/trace.h>

static void module_load(multiboot_tag_t *tag)
{	
  /* calculate size and phy32 pointer */
  size_t size = tag->module.mod_end - tag->module.mod_start;
  elf64_ehdr_t *elf = (elf64_ehdr_t *) aphy32_to_virt(tag->module.mod_start);

  /* make a new process */
  trace_printf("MODULE proc_create()\n");
  proc_t *proc = proc_create();
  if (!proc)
    panic("couldn't create process for module");

  /* switch our address space */
  trace_printf("MODULE proc_switch()\n");
  proc_switch(proc);

  /* load the ELF file */
  trace_printf("MODULE elf64_load()\n");
  if (!elf64_load(elf, size))
    panic("couldn't load elf64 file");

  /* make a new thread */
  trace_printf("MODULE thread_create()\n");
  thread_t *thread = thread_create(proc, 0);
  if (!thread)
    panic("couldn't create thread for module");

  /* set entry point of the thread */
  thread->rip = elf->e_entry;

  /* add thread to the scheduler's ready queue */
  trace_printf("MODULE thread_resume()\n");
  thread_resume(thread);
  
  // The only module is the UI process
  // TODO handle this in a more robust way
  trace_printf("MODULE proc_set_ui_process()\n");
  proc_set_ui_process(proc);
  proc_display(proc->vbeContext);
}

void module_init(multiboot_t *multiboot)
{
  /*
   * disable interrupts, module loading messes around with address space
   * switches so we don't want to confuse the scheduler
   */
  intr_lock();

  /* keep a copy of the old process */
  proc_t *old_proc = proc_get();

  multiboot_tag_t *tag = multiboot_get(multiboot, MULTIBOOT_TAG_MODULE);
  while (tag)
  {
    module_load(tag);
    tag = multiboot_get_after(multiboot, tag, MULTIBOOT_TAG_MODULE);
  }

  /* switch back to the correct address space */
  if (old_proc)
    proc_switch(old_proc);

  /* enable interrupts again */
  intr_unlock();
}
