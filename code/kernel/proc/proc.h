
#ifndef _PROC_PROC_H
#define _PROC_PROC_H

#include <mm/seg.h>
#include <proc/thread.h>
#include <lock/spinlock.h>
#include <util/list.h>
#include <stdint.h>
#include <proc/msg.h>

// Forward declarations
struct proc_node_t;

// Possible process states.
typedef enum
{
  PROC_RUNNING
} proc_state_t;

// Process information block.
typedef struct proc
{
  /* state */
  proc_state_t state;

  /* physical address of the pml4 table of this process */
  uintptr_t pml4_table;

  /* vmm address space lock */
  spinlock_t vmm_lock;

  /* list of threads in this process */
  list_t thread_list;

  /* memory segments */
  seg_t segments;
  
  // VBE context identifier.
  int vbeContext;
  
  // Message queue (FIFO, head is the oldest message).
  list_t messageQueue;
  
  // Internal process list node.
  struct proc_node_t *processListNode;
} proc_t;

// Entry of the internal process list.
struct proc_node_t
{
	// The actual list node.
	struct list_node node;
	
	// Process pointer.
	proc_t *proc;
};

// Defines a message list node.
typedef struct
{
	// Message list node information (used for bookkeeping in proc.c).
	struct list_node node;
	
	// Associated message data.
	msg_header_t *msg;
} msg_node_t;

proc_t *proc_create(void);
proc_t *proc_get(void);

// Set state variables and load the processes virtual address space (the kernel is still mapped into the higher half).
void proc_switch(proc_t *proc);

// Sets the UI process.
void proc_set_ui_process(proc_t *uiProc);

// Displays the process with the given VBE context ID.
void proc_display(int contextId);

// Returns the type of the newest message, or MSG_INVALID if there is none.
msg_type_t proc_peek_message(proc_t *proc);

// Returns the newest message for the given process, and deletes it from the message queue.
msg_header_t *proc_retrieve_message(proc_t *proc);

// Sends a message to the given recipient.
// Do not use this directly! Use msg_send() instead.
void proc_send_message(msg_dest_t dest, msg_header_t *msg);

/*
 * add/remove a thread from this process.
 *
 * these methods should _not_ be called directly. to make a new thread, use
 * thread_create() and specify the proc to attach it to.
 *
 * to stop a thread, use thread_kill(). it will be removed from the process
 * automatically after it is dead.
 */
void proc_thread_add(proc_t *proc, thread_t *thread);
void proc_thread_remove(proc_t *proc, thread_t *thread);

void proc_destroy(proc_t *proc);

#endif
