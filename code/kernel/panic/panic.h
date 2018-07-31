
#ifndef _PANIC_H
#define _PANIC_H

#include <cpu/state.h>
#include <stdarg.h>
#include <stdlib/stdnoreturn.h>

void panic_init(void);
noreturn void panic_handle_ipi(cpu_state_t *state);
noreturn void panic(const char *message, ...);
noreturn void spanic(const char *message, cpu_state_t *state, ...);
noreturn void vpanic(const char *message, va_list args);
noreturn void vspanic(const char *message, cpu_state_t *state, va_list args);

#endif
