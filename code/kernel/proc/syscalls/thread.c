
#include <proc/syscalls.h>
#include <proc/proc.h>
#include <proc/sched.h>
#include <proc/thread.h>
#include <stdlib/stdlib.h>
#include <stdlib/string.h>
#include <panic/panic.h>
#include <trace/trace.h>
#include <proc/elf64.h>
#include <smp/cpu.h>
#include <fs/ramfs.h>

void sys_run_thread(uint64_t rip, const char *name)
{
	// Create thread
	// TODO error checking
	thread_t *thread = thread_create(proc_get(), 0, name);
	thread->rip = rip;
	thread_resume(thread);
}

void sys_exit_thread(cpu_state_t *state)
{
	// Stop thread
	thread_kill(thread_get());
	sched_tick(state);
	
	// TODO clean up
}

bool sys_start_process(const char *programPath)
{
	// Open process executable
	ramfs_fd_t fd;
	if(ramfs_open(programPath, &fd, false) != RAMFS_ERR_OK)
		return false;
	
	// Retrieve file size
	ramfs_seek(0, RAMFS_SEEK_END, fd);
	uint64_t programLength = ramfs_tell(fd);
	
	// Copy executable to memory
	ramfs_seek(0, RAMFS_SEEK_START, fd);
	uint8_t *programKernelMem = malloc(programLength);
	if(ramfs_read(programKernelMem, programLength, fd) != programLength)
	{
		ramfs_close(fd);
		return false;
	}
	ramfs_close(fd);
	elf64_ehdr_t *elf = (elf64_ehdr_t *)programKernelMem;
	
	// Remember handle of current process
	proc_t *currProc = proc_get();
	
	// Create new process and switch address space
	proc_t *proc = proc_create("user_spawned");
	if(!proc)
		panic("Error spawning process during system call");
	proc_switch(proc);
	
	// Load ELF file
	if(!elf64_load(elf, programLength))
	{
		// Show error and restore current process
		trace_printf("Error loading user-supplied ELF file\n");
		proc_switch(currProc);
		return false;
	}
	
	// Spawn thread
	thread_t *thread = thread_create(proc, 0, "user_spawned main");
	thread->rip = elf->e_entry;
	thread_resume(thread);
	
	// Restore current process
	proc_switch(currProc);
	
	// Free memory containing the program executable
	free(programKernelMem);
	
	// Done
	return true;
}

void sys_set_affinity(int coreId)
{
	// ID valid?
	if(coreId < 0 || coreId >= cpuCount)
		return;
	
	// Set affinity of current thread
	thread_t *thread = thread_get();
	thread->coreId = coreId;
}