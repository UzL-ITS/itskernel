
#include <util/list.h>
#include <stdlib/assert.h>

typedef struct
{
  list_node_t *head, *tail;
} pair_t;

void list_init(list_t *list)
{
  list->head = 0;
  list->tail = 0;
  list->size = 0;
}

void list_add_head(list_t *list, list_node_t *node)
{
  if (!list->head)
  {
    assert(list->tail == 0);

    list->head = list->tail = node;
    node->next = node->prev = 0;

    list->size++;
  }
  else
    list_insert_before(list, list->head, node);
}

void list_add_tail(list_t *list, list_node_t *node)
{
  if (!list->tail)
    list_add_head(list, node);
  else
    list_insert_after(list, list->tail, node);
}

void list_insert_before(list_t *list, list_node_t *node, list_node_t *new_node)
{
  new_node->prev = node->prev;
  new_node->next = node;

  if (!node->prev)
    list->head = new_node;
  else
    node->prev->next = new_node;

  node->prev = new_node;

  list->size++;
}

void list_insert_after(list_t *list, list_node_t *node, list_node_t *new_node)
{
  new_node->prev = node;
  new_node->next = node->next;

  if (!node->next)
    list->tail = new_node;
  else
    node->next->prev = new_node;

  node->next = new_node;

  list->size++;
}

void list_remove(list_t *list, list_node_t *node)
{
  if (!node->prev)
    list->head = node->next;
  else
    node->prev->next = node->next;

  if (!node->next)
    list->tail = node->prev;
  else
    node->next->prev = node->prev;

  list->size--;
}

static list_node_t *split(list_node_t *head)
{
  list_node_t *slow = head, *fast = head->next;

  while (fast)
  {
    fast = fast->next;
    if (fast)
    {
      slow = slow->next;
      fast = fast->next;
    }
  }

  list_node_t *midpoint = slow->next;
  slow->next = 0;

  return midpoint;
}

static pair_t merge(list_node_t *left, list_node_t *right, list_compare_t compare)
{
  list_node_t *head = 0, *tail = 0, *ptr;

  while (left || right)
  {
    if (!left)
    {
      ptr = right;
      right = right->next;
    }
    else if (!right)
    {
      ptr = left;
      left = left->next;
    }
    else if ((*compare)(left, right) >= 0)
    {
      ptr = right;
      right = right->next;
    }
    else
    {
      ptr = left;
      left = left->next;
    }

    if (!head)
      head = ptr;
    else
      tail->next = ptr;

    ptr->prev = tail;
    tail = ptr;
  }

  return (pair_t) { .head = head, .tail = tail };
}

static pair_t mergesort(list_node_t *head, list_compare_t compare)
{
  if (!head)
    return (pair_t) { .head = head, .tail = 0 };
  if (!head->next)
    return (pair_t) { .head = head, .tail = head->next };

  list_node_t *midpoint = split(head);

  pair_t left = mergesort(head, compare);
  pair_t right = mergesort(midpoint, compare);

  return merge(left.head, right.head, compare);
}

void list_sort(list_t *list, list_compare_t compare)
{
  pair_t pair = mergesort(list->head, compare);
  list->head = pair.head;
  list->tail = pair.tail;
}
