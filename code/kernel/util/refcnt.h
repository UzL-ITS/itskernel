
#ifndef _UTIL_REFCNT_H
#define _UTIL_REFCNT_H

typedef struct
{
  unsigned int count;
} refcnt_t;

void refcnt_init(refcnt_t *refcnt);
void refcnt_retain(refcnt_t *refcnt);
void refcnt_release(refcnt_t *refcnt);

#endif
