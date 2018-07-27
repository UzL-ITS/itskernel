
#ifndef _UTIL_LIST_H
#define _UTIL_LIST_H

typedef struct list_node
{
  struct list_node *next, *prev;
} list_node_t;

typedef struct
{
  list_node_t *head, *tail;
  int size;
} list_t;

typedef int (*list_compare_t)(const void *left, const void *right);

#define LIST_EMPTY { .head = 0, .tail = 0, .size = 0 }

#define list_for_each(list, node) for (list_node_t *node = (list)->head, *__next = node ? node->next : 0; node; node = __next, __next = node ? node->next : 0)

void list_init(list_t *list);
void list_add_head(list_t *list, list_node_t *node);
void list_add_tail(list_t *list, list_node_t *node);
void list_insert_before(list_t *list, list_node_t *node, list_node_t *new_node);
void list_insert_after(list_t *list, list_node_t *node, list_node_t *new_node);
void list_remove(list_t *list, list_node_t *node);
void list_sort(list_t *list, list_compare_t compare);

#endif
