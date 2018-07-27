
#ifndef _MM_VALIDATE_H
#define _MM_VALIDATE_H

#include <stdbool.h>
#include <stddef.h>

/*
 * Check if a buffer is wholly contained within user-space. All pointers passed
 * to a system call must be checked with this function, to avoid security
 * issues, whereby a rogue user program could manipulate the kernel into
 * reading or corrupting kernel memory for it.
 */
bool valid_buffer(const void *ptr, size_t len);

/*
 * Checks if a null-terminated string is wholly contained within user-space,
 * all strings passed to a system call must be likewise checked with this
 * function.
 */
bool valid_string(const char *str);

#endif
