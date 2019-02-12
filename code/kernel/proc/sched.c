
#include <proc/sched.h>
#include <cpu/halt.h>
#include <proc/proc.h>
#include <smp/cpu.h>
#include <smp/mode.h>
#include <time/apic.h>
#include <time/pit.h>
#include <util/container.h>
#include <util/list.h>
#include <panic/panic.h>
#include <stdlib/string.h>
#include <stdbool.h>
#include <io/keyboard.h>

#define SCHED_TIMESLICE 10 /* 10ms = 100Hz */

/* a list of threads that are ready to run */
static list_t thread_queue = LIST_EMPTY;
static spinlock_t thread_queue_lock = SPIN_UNLOCKED;

// Determines whether the scheduler interrupt handler has been installed yet.
static bool interruptInstalled = false;
static spinlock_t interruptInstalledLock = SPIN_UNLOCKED;

// Time point of the last keyboard poll.
static uint64_t lastKeyboardPoll = 0;
#define SCHED_KEYBOARD_POLL_DELAY 80

// Handles a timer interrupt.
static void sched_handle_interrupt(cpu_state_t *state)
{
	// Increment timer counter
	cpu_t *cpu = cpu_get();
	cpu->elapsedMsSinceStart += SCHED_TIMESLICE;
	
	// Keyboard workaround
	if(cpu->bsp)
	{
		// Do not poll too often
		if(cpu->elapsedMsSinceStart - lastKeyboardPoll >= SCHED_KEYBOARD_POLL_DELAY)
		{
			lastKeyboardPoll = cpu->elapsedMsSinceStart;
			keyboard_poll();
		}
	}

	// Process scheduler tick
	sched_tick(state);
}

void sched_init(bool bsp)
{
	// Assign interrupt handler, if not already done
	spin_lock(&interruptInstalledLock);
	if(!interruptInstalled)
	{
		// Install handler
		if(smp_mode == MODE_SMP)
			apic_timer_install_handler(sched_handle_interrupt);
		else
			pit_timer_install_handler(sched_handle_interrupt);
		interruptInstalled = true;
	}
	spin_unlock(&interruptInstalledLock);
		
	// Start scheduler timer for this CPU
	if(smp_mode == MODE_SMP)
		apic_monotonic(SCHED_TIMESLICE);
	else
		pit_monotonic(SCHED_TIMESLICE);
}

void sched_thread_resume(thread_t *thread)
{
	spin_lock(&thread_queue_lock);
	list_add_tail(&thread_queue, &thread->sched_node);
	spin_unlock(&thread_queue_lock);
}

void sched_thread_suspend(thread_t *thread)
{
	spin_lock(&thread_queue_lock);
	list_remove(&thread_queue, &thread->sched_node);
	spin_unlock(&thread_queue_lock);
}

void sched_tick(cpu_state_t *state)
{
	cpu_t *cpu = cpu_get();

	/* figure out what thread is currently running on the CPU */
	thread_t *currThread = cpu->thread;

	spin_lock(&thread_queue_lock);

	/* add the current thread to the queue if it is runnable */
	if(currThread && currThread->state == THREAD_RUNNING)
		list_add_tail(&thread_queue, &currThread->sched_node);
	
	/* pick the next thread to run */
	list_node_t *nextThreadNode = thread_queue.head;
	thread_t *nextThread = 0;
	while(nextThreadNode)
	{
		// Get thread object
		thread_t *nextThreadCandidate = container_of(nextThreadNode, thread_t, sched_node);
		
		// Thread runnable by current core?
		if(nextThreadCandidate->coreId != cpu->coreId)
		{
			// Check next thread
			nextThreadNode = nextThreadNode->next;
			continue;
		}
		
		// Thread can be run, remove it from the queue
		nextThread = nextThreadCandidate;
		list_remove(&thread_queue, nextThreadNode);
		break;
	}
	
	// TODO the register file copy below causes a race condition, when another core picks up execution of currThread before the whole state was copied
	//      Temporary fix: Lock the entire scheduler step
	//spin_unlock(&thread_queue_lock);

	/* if there is no new thread, switch to the idle thread */
	if(!nextThreadNode)
		nextThread = cpu->idle_thread;

	/* check if we're actually switching threads */
	if(currThread != nextThread)
	{
		/* actually swap the pointers over */
		cpu->thread = nextThread;

		/* save the register file for the current thread */
		if(currThread)
		{
			memcpy(currThread->regs, state->regs, sizeof(state->regs));
			currThread->rip = state->rip;
			currThread->rsp = state->rsp;
			currThread->rflags = state->rflags;
			currThread->cs = state->cs;
			currThread->ss = state->ss;
		}

		/* restore the register file for the new thread */
		memcpy(state->regs, nextThread->regs, sizeof(state->regs));
		state->rip = nextThread->rip;
		state->rsp = nextThread->rsp;
		state->rflags = nextThread->rflags;
		state->cs = nextThread->cs;
		state->ss = nextThread->ss;

		/* if we're switcing between processes, we need to switch address spaces */
		if(!currThread || currThread->proc != nextThread->proc)
			proc_switch(nextThread->proc); /* (this also sets cpu->proc) */

		/* write new kernel stack pointer into the TSS */
		tss_set_rsp0(nextThread->kernel_rsp);
	}

	spin_unlock(&thread_queue_lock);
}
