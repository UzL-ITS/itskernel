
#ifndef _MM_MAP_H
#define _MM_MAP_H

#include <stddef.h>
#include <stdint.h>
#include <init/multiboot.h>
#include <util/list.h>

typedef struct
{
  struct list_node node;
  int type;
  uintptr_t addr_start;
  uintptr_t addr_end;
} mm_map_entry_t;

list_t *mm_map_init(multiboot_t *multiboot);

#endif
