
#include <panic/panic.h>
#include <trace/stacktrace.h>
#include <trace/trace.h>
#include <smp/mode.h>
#include <cpu/cr.h>
#include <cpu/efer.h>
#include <cpu/intr.h>
#include <cpu/halt.h>
#include <intr/common.h>
#include <intr/route.h>
#include <intr/apic.h>
#include <vbe/vbe.h>

void panic_init(void)
{
  if (!intr_route_intr(IPI_PANIC, &panic_handle_ipi))
    panic("failed to route panic IPI");
}

noreturn void panic_handle_ipi(cpu_state_t *state)
{
  intr_disable();
  halt_forever();
}

noreturn void panic(const char *message, ...)
{
  va_list args;
  va_start(args, message);
  vspanic(message, 0, args);
  va_end(args);
}

noreturn void spanic(const char *message, cpu_state_t *state, ...)
{
  va_list args;
  va_start(args, state);
  vspanic(message, state, args);
  va_end(args);
}

noreturn void vpanic(const char *message, va_list args)
{
  vspanic(message, 0, args);
}

noreturn void vspanic(const char *message, cpu_state_t *state, va_list args)
{
	// Switch VBE context to ensure that the error message is visible
	vbe_show_context(0);
	
	if(smp_mode == MODE_SMP)
		apic_ipi_all_exc_self(IPI_PANIC);

	trace_puts("PANIC: ");
	trace_vprintf(message, args);
	trace_puts("\n\n");

	if(state)
	{
		trace_puts(" Registers:\n");
		trace_printf("  RAX=%0#18x RBX=%0#18x RCX=%0#18x\n", state->regs[RAX], state->regs[RBX], state->regs[RCX]);

		trace_printf("  RDX=%0#18x RSI=%0#18x RDI=%0#18x\n", state->regs[RDX], state->regs[RSI], state->regs[RDI]);

		trace_printf("  RBP=%0#18x RSP=%0#18x R8 =%0#18x\n", state->regs[RBP], state->rsp, state->regs[R8]);

		trace_printf("  R9 =%0#18x R10=%0#18x R11=%0#18x\n", state->regs[R9], state->regs[R10], state->regs[R11]);

		trace_printf("  R12=%0#18x R13=%0#18x R14=%0#18x\n", state->regs[R12], state->regs[R13], state->regs[R15]);

		trace_printf("  R15=%0#18x RIP=%0#18x RFL=%0#18x\n\n", state->regs[R15], state->rip, state->rflags);


		trace_printf("  CR0=%0#18x CR2=%0#18x CR3=%0#18x\n", cr0_read(), cr2_read(), cr3_read());

		trace_printf("  CR4=%0#18x EFR=%0#18x CS =%0#6x  SS =%0#6x\n\n", cr4_read(), efer_read(), state->cs, state->ss);
	}

	trace_puts(" Stack Trace:\n");
	stacktrace_emit();

	intr_disable();
	halt_forever();
}
