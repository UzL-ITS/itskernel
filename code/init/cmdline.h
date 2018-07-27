
#ifndef _CMDLINE_H
#define _CMDLINE_H

#include <init/multiboot.h>

void cmdline_init(multiboot_t *multiboot);
const char *cmdline_get(const char *key);

#endif
