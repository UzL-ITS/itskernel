
#ifndef _PROC_SYSCALL_H
#define _PROC_SYSCALL_H

#include <stdint.h>

extern uintptr_t syscall_table[];
extern uint64_t syscall_table_size;

void syscall_init(void);
void syscall_stub(void);

#endif
