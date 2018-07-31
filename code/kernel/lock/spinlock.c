
#include <lock/spinlock.h>
#include <cpu/pause.h>
#include <lock/intr.h>
#include <stdlib/assert.h>

void spin_lock(spinlock_t *lock)
{
  for (;;)
  {
    if (spin_try_lock(lock))
      break;

    pause_once();
  }
}

bool spin_try_lock(spinlock_t *lock)
{
  intr_lock();

  if (__sync_bool_compare_and_swap(lock, SPIN_UNLOCKED, SPIN_LOCKED))
    return true;

  intr_unlock();
  return false;
}

void spin_unlock(spinlock_t *lock)
{
  assert(__sync_bool_compare_and_swap(lock, SPIN_LOCKED, SPIN_UNLOCKED));
  intr_unlock();
}
