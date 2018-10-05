
#include <intr/fault.h>
#include <intr/common.h>
#include <intr/route.h>
#include <panic/panic.h>
#include <mm/common.h>
#include <proc/thread.h>
#include <trace/trace.h>
#include <smp/cpu.h>

enum fault_codes
{
	FAULT_PAGE_FAULT = 14,
	FAULT_GENERAL_PROTECTION_FAULT = 13
};

static const char *fault_names[] = {
  "Divide by Zero Error",
  "Debug",
  "Non Maskable Interrupt",
  "Breakpoint",
  "Overflow",
  "Bound Range",
  "Invalid Opcode",
  "Device Not Available",
  "Double Fault",
  "Coprocessor Segment Overrun",
  "Invalid TSS",
  "Segment Not Present",
  "Stack-Segment Fault",
  "General Protection Fault",
  "Page Fault",
  "Reserved",
  "x87 Floating-Point Exception",
  "Alignment Check",
  "Machine Check",
  "SIMD Floating-Point Exception",
  "Reserved",
  "Reserved",
  "Reserved",
  "Reserved",
  "Reserved",
  "Reserved",
  "Reserved",
  "Reserved",
  "Reserved",
  "Reserved",
  "Security Exception",
  "Reserved"
};

void fault_init(void)
{
  for (intr_t intr = FAULT0; intr <= FAULT31; intr++)
  {
    /* skip routing NMI, this is done in nmi.c */
    if (intr != FAULT2)
    {
      if (!intr_route_intr(intr, &fault_handle))
        panic("failed to route FAULT%d", intr);
    }
  }
}

void fault_handle(cpu_state_t *state)
{
	// Handleable fault?
	switch(state->id)
	{
		case FAULT_PAGE_FAULT:
		{
			// User space?
			if(state->rip <= VM_USER_END)
			{
				// Print part of user space stack first, if possible
				thread_t *thread = thread_get();
				int dumpedStackEntries = 16;
				uint64_t *rsp = (uint64_t *)state->rsp;
				uint64_t *threadStackBase = (uint64_t *)((uint64_t)thread->stack + USER_STACK_SIZE);
				trace_printf("User space page fault, RSP = %016x (base %016x), dumping first %d stack entries:\n", rsp, threadStackBase, dumpedStackEntries);
				for(int i = 0; i < dumpedStackEntries; ++i)
				{
					// Print next stack entry, if there is any
					if(rsp >= threadStackBase)
						break;
					trace_printf("    %016x\n", *rsp);
					++rsp;
				}
			}
		
			// Call generic handler with kernel stack trace output
			cpu_t *cpu = cpu_get();
			spanic("Page fault: num=%d, error=%0#18x, core=%d", state, state->id, state->error, cpu->coreId, state->rip);
			break;
		}
		
		case FAULT_GENERAL_PROTECTION_FAULT:
		{
			// Is there any active user space thread?
			thread_t *thread = thread_get();
			if(thread)
			{
				// Print part of user space stack first, if possible
				int dumpedStackEntries = 16;
				uint64_t *rsp = (uint64_t *)state->rsp;
				uint64_t *threadStackBase = (uint64_t *)((uint64_t)thread->stack + USER_STACK_SIZE);
				trace_printf("General protection fault, user space thread stack at %016x, dumping first %d stack entries:\n", threadStackBase, dumpedStackEntries);
				for(int i = 0; i < dumpedStackEntries; ++i)
				{
					// Print next stack entry, if there is any
					if(rsp >= threadStackBase)
						break;
					trace_printf("    %016x\n", *rsp);
					++rsp;
				}
			}
		
			// Call generic handler with kernel stack trace output
			cpu_t *cpu = cpu_get();
			spanic("General protection fault: num=%d, error=%0#18x, core=%d", state, state->id, state->error, cpu->coreId, state->rip);
			break;
		}
		
		default:
		{
			// Generic panic handler with CPU state
			const char *name = fault_names[state->id];
			spanic("Fault: %s (num=%d, error=%0#18x)", state, name, state->id, state->error, state->rip);
			break;
		}
	}
}
