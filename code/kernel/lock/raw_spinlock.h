#pragma once

/*
Interrupt-free spin lock. This does not disable interrupts on lock, so be careful not to cause deadlocks!
*/

/* INCLUDES */

#include <stdint.h>


/* TYPES */

// Type of the spin lock.
typedef uint64_t raw_spinlock_t;

// Initial value of a new raw spinlock (not acquired).
#define RAW_SPINLOCK_UNLOCKED 1


/* DECLARATIONS */

// Acquires the given mutex; if it is already acquired by another thread, this function blocks.
void raw_spinlock_lock(raw_spinlock_t *raw_spinlock);

// Releases the given mutex.
void raw_spinlock_unlock(raw_spinlock_t *raw_spinlock);