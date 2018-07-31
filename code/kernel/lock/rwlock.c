
#include <lock/rwlock.h>
#include <lock/intr.h>
#include <limits.h>

void rw_rlock(rwlock_t *lock)
{
  bool acquired = false;
  do
  {
    spin_lock(&lock->lock);
    if (!lock->writing && lock->read_permits != INT_MAX)
    {
      lock->read_permits++;
      acquired = true;
      intr_lock();
    }
    spin_unlock(&lock->lock);
  } while (!acquired);
}

void rw_runlock(rwlock_t *lock)
{
  spin_lock(&lock->lock);
  lock->read_permits--;
  intr_unlock();
  spin_unlock(&lock->lock);
}

void rw_wlock(rwlock_t *lock)
{
  bool acquired = false;
  do
  {
    spin_lock(&lock->lock);
    if (!lock->writing && lock->read_permits == 0)
    {
      lock->writing = true;
      acquired = true;
      intr_lock();
    }
    spin_unlock(&lock->lock);
  } while (!acquired);
}

void rw_wunlock(rwlock_t *lock)
{
  spin_lock(&lock->lock);
  lock->writing = false;
  intr_unlock();
  spin_unlock(&lock->lock);
}
