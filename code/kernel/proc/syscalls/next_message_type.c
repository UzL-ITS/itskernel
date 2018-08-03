
#include <proc/syscalls.h>
#include <proc/msg.h>
#include <proc/proc.h>

msg_type_t sys_next_message_type()
{
	// Get current process
	proc_t *proc = proc_get();
	
	// Retrieve message type
	return proc_peek_message(proc);
}