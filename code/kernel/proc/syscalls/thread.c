
#include <proc/syscalls.h>
#include <proc/proc.h>
#include <proc/sched.h>
#include <proc/thread.h>

void sys_run_thread(uint64_t rip)
{
	// Create thread
	// TODO error checking
	thread_t *thread = thread_create(proc_get(), 0);
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