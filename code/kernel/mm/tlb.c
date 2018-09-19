
#include <mm/tlb.h>
#include <cpu/tlb.h>
#include <cpu/intr.h>
#include <intr/common.h>
#include <intr/route.h>
#include <intr/apic.h>
#include <lock/spinlock.h>
#include <smp/cpu.h>
#include <smp/mode.h>
#include <panic/panic.h>
#include <stdbool.h>
#include <stddef.h>
#include <util/container.h>
#include <trace/trace.h>

#define TLB_OP_QUEUE_SIZE 16

// Lock for accessing the CPU TLB operation queue.
static spinlock_t tlbQueueLock = SPIN_UNLOCKED;

// Handles the TLB operations for the current CPU.
static void tlb_handle_ops(void)
{
	spin_lock(&tlbQueueLock);
	
	// Get CPU TLB queue
	cpu_t *cpu = cpu_get();
	for(int i = 0; i < cpu->tlbOperationQueueLength; ++i)
	{
		// Act depending on operation type
		tlb_op_t *op = &cpu->tlbOperationQueue[i];
		switch(op->type)
		{
			// Invalidate single entry
			case TLB_OP_INVLPG:
				tlb_invlpg(op->addr);
				break;

			// Flush whole TLB
			case TLB_OP_FLUSH:
				tlb_flush();
				break;
		}
	}
	
	// All queue entries were consumed
	cpu->tlbOperationQueueLength = 0;

	spin_unlock(&tlbQueueLock);
}

// Interrupt service routine for the TLB IPI.
static void tlb_handle_ipi(cpu_state_t *state)
{
	// Handle this CPU's TLB queue
	tlb_handle_ops();
}

void tlb_init(void)
{
	// Install TLB IPI interrupt
	if(!intr_route_intr(IPI_TLB, &tlb_handle_ipi))
		panic("failed to route TLB shootdown IPI");
}

void tlb_transaction_init(void)
{
	// Acquire TLB lock
	spin_lock(&tlbQueueLock);
}		

void tlb_transaction_queue_invlpg(uintptr_t addr)
{
	// Run through CPUs and add invalidate operations to their queues
	list_for_each(&cpu_list, cpuNode)
	{
		// Get current CPU
		cpu_t *cpu = container_of(cpuNode, cpu_t, node);
		
		// Queue full? => Just replace everything with a flush operation
		if(cpu->tlbOperationQueueLength == TLB_OP_QUEUE_SIZE)
		{
			// Flush whole TLB
			tlb_op_t *op = &cpu->tlbOperationQueue[0];
			op->type = TLB_OP_FLUSH;
			cpu->tlbOperationQueueLength = 1;
		}
		else if(cpu->tlbOperationQueueLength == 1 && cpu->tlbOperationQueue[0].type == TLB_OP_FLUSH)
		{
			// There is already a flush enqueued, do nothing
		}
		else
		{
			// Add invalidate entry
			tlb_op_t *op = &cpu->tlbOperationQueue[cpu->tlbOperationQueueLength++];
			op->type = TLB_OP_INVLPG;
			op->addr = addr;
		}
	}
}

void tlb_transaction_commit(void)
{
	// TLB transaction done, release lock
	spin_unlock(&tlbQueueLock);
	
	// Handle queue on CPU doing the TLB transaction
	tlb_handle_ops();

	// Notify other CPUs to handle their TLB queues
	if(smp_mode == MODE_SMP)
		apic_ipi_all_exc_self(IPI_TLB);
}
