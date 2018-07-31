
#ifndef _LOCK_RWLOCK_H
#define _LOCK_RWLOCK_H

#include <lock/spinlock.h>
#include <stdbool.h>

#define RWLOCK_UNLOCKED { .read_permits = 0, .writing = false, .lock = SPIN_UNLOCKED }

typedef struct
{
  /* The number of threads which have acquired the lock in read-only mode. */
  int read_permits;

  /* A flag indicating if this lock has been acquired in read-write mode. */
  bool writing;

  /*
   * The spinlock that allows atomic operations to be performed on the above
   * fields.
   */
  spinlock_t lock;
} rwlock_t;

/*
 * Acquires the given read-write lock in read-only mode. If the lock is
 * unlocked, or if the lock has already been acquired for reading, it is
 * acquired immediately.
 *
 * Otherwise, this code will block by spinning until the write lock is
 * released, then acquire the read lock.
 *
 * Once acquired, interrupts are also disabled, so this lock is safe to be
 * used inside an interrupt handler.
 *
 * Obtaining the lock in read-only mode is re-entrant, unlike write-only mode.
 */
void rw_rlock(rwlock_t *lock);

/* Releases the read lock. */
void rw_runlock(rwlock_t *lock);

/*
 * Acquires the given read-write lock in read-write mode. This code blocks by
 * spinning until the lock is not locked in either read-only nor read-write
 * mode, and then acquires the lock. Interrupts are disabled upon the lock
 * being acquired, as described above.
 */
void rw_wlock(rwlock_t *lock);

/* Releases the write lock. */
void rw_wunlock(rwlock_t *lock);

#endif
