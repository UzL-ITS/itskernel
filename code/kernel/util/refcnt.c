
#include <util/refcnt.h>
#include <stdlib/assert.h>

void refcnt_init(refcnt_t *refcnt)
{
  refcnt->count = 1;
}

void refcnt_retain(refcnt_t *refcnt)
{
  // TODO maybe have some sanity checking for reaching UINT_MAX?
  refcnt->count++;
}

void refcnt_release(refcnt_t *refcnt)
{
  assert(refcnt->count != 0);
  refcnt->count--;
}
