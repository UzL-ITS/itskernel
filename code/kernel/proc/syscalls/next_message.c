
#include <proc/syscalls.h>
#include <proc/msg.h>
#include <proc/proc.h>
#include <stdlib/string.h>

void sys_next_message(msg_header_t *messageBuffer)
{
	// Get current process
	proc_t *proc = proc_get();
	
	// Retrieve message
	msg_header_t *msg = proc_retrieve_message(proc);
	
	// Copy message into user space buffer
	memcpy(messageBuffer, msg, msg->size);
	
	// Free memory of retrieved message (handling is done by the user now)
	msg_free(msg);
}