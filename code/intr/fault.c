
#include <intr/fault.h>
#include <intr/common.h>
#include <intr/route.h>
#include <panic/panic.h>

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
  const char *name = fault_names[state->id];
  spanic("Fault: %s (num=%d, error=%0#18x)", state, name, state->id, state->error, state->rip);
}
