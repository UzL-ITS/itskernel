
#include <proc/syscalls.h>
#include <proc/proc.h>

void sys_set_displayed_process(int contextId)
{
	// Change render context
	proc_display(contextId);
}