
#ifndef _STACKTRACE_H
#define _STACKTRACE_H

#include <init/multiboot.h>

void stacktrace_init(multiboot_t *multiboot);
void stacktrace_emit(void);

#endif
