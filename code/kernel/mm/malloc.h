
#ifndef _MM_MALLOC_H
#define _MM_MALLOC_H

#include <panic/panic.h>
#include <mm/common.h>
#include <lock/spinlock.h>
#include <stddef.h>

/* make it so when dlmalloc aborts a kernel panic is triggered */
#define ABORT panic("dlmalloc abort()")

/* use assert() assert.h for assertions */
#define ABORT_ON_ASSERT_FAILURE 0

/* these headers do not exist (or are not complete) in the kernel */
#define LACKS_ERRNO_H
#define LACKS_STDLIB_H
#define LACKS_TIME_H
#define LACKS_STRINGS_H
#define LACKS_SYS_TYPES_H
#define LACKS_SYS_MMAN_H
#define LACKS_SYS_PARAM_H
#define LACKS_FCNTL_H
#define LACKS_UNISTD_H
#define LACKS_SCHED_H

/* stats require stdio.h which we don't support */
#define NO_MALLOC_STATS 1

/* don't use built-in spinlocks */
#define USE_LOCKS           0
#define USE_SPIN_LOCKS      0

/* prefix function names with dl */
#define USE_DL_PREFIX 1

/* we don't support errno so just nop instead */
#define MALLOC_FAILURE_ACTION

/* we only support mmap-like calls */
#define HAVE_MORECORE 0
#define HAVE_MMAP     1
#define HAVE_MREMAP   0
#define MMAP_CLEARS   0

/* hard-coded page size of 4k */
#define malloc_getpagesize FRAME_SIZE

/* some error codes */
#define ENOMEM 1
#define EINVAL 2

/* minimal definitions for translating mmap() to heap_*() */
#define MAP_ANONYMOUS 0x1
#define MAP_PRIVATE   0x2

#define MAP_FAILED ((void *) -1)

#define PROT_NONE  0x0
#define PROT_READ  0x1
#define PROT_WRITE 0x2
#define PROT_EXEC  0x4

typedef size_t off_t;

extern spinlock_t malloc_lock;

void *mmap(void *addr, size_t len, int prot, int flags, int fd, off_t off);
int munmap(void *addr, size_t len);

/* include the actual dlmalloc header file */
#include <mm/dlmalloc.h>

#endif
