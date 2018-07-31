
#ifndef _INTR_FAULT_H
#define _INTR_FAULT_H

#include <cpu/state.h>

void fault_init(void);
void fault_handle(cpu_state_t *state);

#endif
