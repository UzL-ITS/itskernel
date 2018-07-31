
#ifndef _UTIL_CONTAINER_H
#define _UTIL_CONTAINER_H

#define container_of(ptr, type, member) ((type *) (((char *) (ptr)) - offsetof(type, member)))

#endif
