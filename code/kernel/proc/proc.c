
#include <proc/proc.h>
#include <cpu/cr.h>
#include <smp/cpu.h>
#include <mm/pmm.h>
#include <mm/vmm.h>
#include <lock/intr.h>
#include <stdlib/stdlib.h>
#include <vbe/vbe.h>
#include <util/list.h>
#include <util/container.h>
#include <lock/spinlock.h>
#include <panic/panic.h>
#include <trace/trace.h>

// The currently displayed process.
proc_t *processDisplayed = 0;

// The UI process.
proc_t *uiProcess = 0;

// The currently active processes.
list_t processList = LIST_EMPTY;

// Lock to ensure ordered access to the process list and the currently displayed process.
static spinlock_t processListLock = SPIN_UNLOCKED;

proc_t *proc_create(void)
{
	// Allocate necessary objects
	proc_t *proc = malloc(sizeof(*proc));
	struct proc_node_t *procNode = malloc(sizeof(struct proc_node_t));
	if(!proc || !procNode)
		return 0;

	// Allocate PML4 table for this process
	proc->pml4_table = pmm_alloc();
	if(!proc->pml4_table)
	{
		free(proc);
		free(procNode);
		return 0;
	}

	// Initialize PML4 table with higher-half kernel-space entries
	if(!vmm_init_pml4(proc->pml4_table))
	{
		pmm_free(proc->pml4_table);
		free(proc);
		free(procNode);
		return 0;
	}

	// Initialize process virtual memory
	proc->vmm_lock = SPIN_UNLOCKED;
	if(!seg_init(&proc->segments))
	{
		pmm_free(proc->pml4_table);
		free(proc);
		free(procNode);
		return 0;
	}

	// Create VBE context for this process
	proc->vbeContext = vbe_create_context();

	// Initialize empty message queue
	list_init(&proc->messageQueue);

	// Initialize empty thread list
	list_init(&proc->thread_list);

	// Store it in internal list for bookkeeping
	procNode->proc = proc;
	spin_lock(&processListLock);
	{
		list_add_tail(&processList, &procNode->node);
	}
	spin_unlock(&processListLock);
	proc->processListNode = procNode;
	
	// Process is up and running
	proc->state = PROC_RUNNING;
	return proc;
}

proc_t *proc_get(void)
{
	cpu_t *cpu = cpu_get();
	return cpu->proc;
}

void proc_set_ui_process(proc_t *uiProc)
{
	uiProcess = uiProc;
}

void proc_display(int contextId)
{
	spin_lock(&processListLock);
	{	
		// Context #0 is the kernel itself
		// Show its context, but keep the processDisplayed variable as it is, to be able to route keyboard inputs
		if(contextId == 0)
		{
			vbe_show_context(0);
			spin_unlock(&processListLock);
			return;
		}
		
		// Find process with given context ID
		list_for_each(&processList, procNode)
		{
			// Retrieve process object
			proc_t *proc = container_of(procNode, struct proc_node_t, node)->proc;
			
			// Check VBE context ID
			if(proc->vbeContext == contextId)
			{
				// Display process
				vbe_show_context(contextId);
				processDisplayed = proc;
				break;
			}
		}
	}
	spin_unlock(&processListLock);
}

void proc_send_message(msg_dest_t dest, msg_header_t *msg)
{
	// Allocate node for storing the message
	msg_node_t *msgNode = (msg_node_t *)malloc(sizeof(msg_node_t));
	msgNode->msg = msg;
	
	// Retrieve target process
	proc_t *destProc;
	switch(dest)
	{
		case MSG_DEST_UI_PROCESS:
		{
			destProc = uiProcess;
			break;
		}
		
		case MSG_DEST_VISIBLE_PROCESS:
		{
			spin_lock(&processListLock);
			{
				destProc = processDisplayed;
			}
			spin_unlock(&processListLock);
			break;
		}
		
		default:
			// ???
			return;
	}
	if(!destProc)
		panic("Could not find any process to send the given message to!\n");
	
	// Add message to queue
	// TODO use non-interrupt lock
	intr_lock();
	{
		list_add_tail(&destProc->messageQueue, &msgNode->node);
	}
	intr_unlock();
}

msg_type_t proc_peek_message(proc_t *proc)
{
	// Lock interrupts so we don't end up in a deadlock when e.g. a key is pressed and the handler tries to add a new message
	msg_node_t *msgNode;
	intr_lock();
	{
		msgNode = container_of(proc->messageQueue.head, msg_node_t, node);
	}
	intr_unlock();
	
	// Return message type
	if(msgNode)
		return msgNode->msg->type;
	return MSG_INVALID;
}

msg_header_t *proc_retrieve_message(proc_t *proc)
{
	// Lock interrupts so we don't end up in a deadlock when e.g. a key is pressed and the handler tries to add a new message
	msg_node_t *msgNode;
	intr_lock();
	{
		msgNode = container_of(proc->messageQueue.head, msg_node_t, node);
		if(msgNode)
			list_remove(&proc->messageQueue, &msgNode->node);
	}
	intr_unlock();
	
	// Message found?
	if(msgNode)
	{
		// Retrieve message
		msg_header_t *msg = msgNode->msg;
		
		// Free node memory
		free(msgNode);
		
		// Done
		return msg;
	}
	
	// No message present
	return 0;
}

void proc_switch(proc_t *proc)
{
	// Set current process and load address space
	cpu_t *cpu = cpu_get();
	cpu->proc = proc;
	cr3_write(proc->pml4_table);
}

void proc_thread_add(proc_t *proc, thread_t *thread)
{
	list_add_tail(&proc->thread_list, &thread->proc_node);
}

void proc_thread_remove(proc_t *proc, thread_t *thread)
{
	list_remove(&proc->thread_list, &thread->proc_node);
}

void proc_destroy(proc_t *proc)
{
	// TODO: destroy threads within the process and make sure they aren't queued

	// Delete process node
	spin_lock(&processListLock);
	{
		list_remove(&processList, &proc->processListNode->node);
	}
	spin_unlock(&processListLock);
	free(proc->processListNode);
	
	// Make sure process is not displayed anymore
	if(processDisplayed == proc)
		proc_display(0);
	// TODO delete VBE context

	/* lock interrupts so we can temporarily switch address spaces */
	intr_lock();

	/* record the old pml4 table and switch to the new one */
	uintptr_t old_pml4_table = cr3_read();
	cr3_write(proc->pml4_table);

	/* destroy the user memory segments */
	seg_destroy();

	/* switch back to the old address space and unlock interrupts */
	cr3_write(old_pml4_table);
	intr_unlock();

	// TODO: if cpu->proc == proc, change it to 0?

	/* free the pml4 table and process struct */
	pmm_free(proc->pml4_table);
	free(proc);
}
