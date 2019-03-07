
#include <proc/module.h>
#include <cpu/cr.h>
#include <lock/intr.h>
#include <proc/proc.h>
#include <proc/elf64.h>
#include <mm/phy32.h>
#include <panic/panic.h>
#include <stddef.h>
#include <trace/trace.h>
#include <stdlib/string.h>
#include <fs/ramfs.h>

static void module_load(multiboot_tag_t *tag)
{	
	// Calculate size and phy32 pointer
	size_t size = tag->module.mod_end - tag->module.mod_start;
	uint8_t *modPtr = (uint8_t *)aphy32_to_virt(tag->module.mod_start);
	
	// The UI process is loaded directly, while other apps are provided via the file system
	char moduleName[250];
	strcpy(moduleName, tag->module.string);
	if(strcmp(moduleName, "ui") == 0)
	{
		// Load binary
		trace_printf("Found UI process binary, loading...\n");
		elf64_ehdr_t *elf = (elf64_ehdr_t *)modPtr;
		
		// Make a new process
		proc_t *proc = proc_create("module");
		if (!proc)
			panic("Couldn't create UI process");

		// Switch our address space
		proc_switch(proc);

		// Load the ELF file
		if (!elf64_load(elf, size))
			panic("Couldn't load UI elf64 file");

		// Make a new thread
		thread_t *thread = thread_create(proc, 0, "ui_main");
		if (!thread)
			panic("Couldn't create UI main thread");

		// Set entry point of the thread
		thread->rip = elf->e_entry;

		// Add thread to the scheduler's ready queue
		thread_resume(thread);
		
		// Set UI process
		proc_set_ui_process(proc);
		proc_display(proc->vbeContext);
		trace_printf("UI process successfully loaded.\n");
	}
	else
	{
		// Make sure the /apps subdirectory exists
		if(ramfs_test_directory("/", "apps") == RAMFS_ERR_DIRECTORY_DOES_NOT_EXIST)
			ramfs_create_directory("/", "apps");
		
		// Assemble file name for binary
		char fileName[6 + sizeof(moduleName)] = "/apps/";
		strcpy(&fileName[6], moduleName);
		
		// Copy module data into new file
		ramfs_fd_t fd;
		if(ramfs_open(fileName, &fd, true) != RAMFS_ERR_OK)
			trace_printf("Copying module '%s' failed\n", moduleName);
		else
		{
			// Write data
			ramfs_write(modPtr, size, fd);
			ramfs_close(fd);
		}
	}
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
