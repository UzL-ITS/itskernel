
#ifndef _LOCK_SPINLOCK_H
#define _LOCK_SPINLOCK_H

#include <stdbool.h>
#include <stdint.h>

#define SPIN_UNLOCKED 0
#define SPIN_LOCKED   1

typedef uint64_t spinlock_t;

void spin_lock(spinlock_t *lock);
bool spin_try_lock(spinlock_t *lock);
void spin_unlock(spinlock_t *lock);

#endif
