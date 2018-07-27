
#include <init/cmdline.h>
#include <mm/seq.h>
#include <util/container.h>
#include <util/list.h>
#include <panic/panic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib/string.h>

typedef struct cmdline_pair
{
  list_node_t node;
  const char *key, *value;
} cmdline_pair_t;

static list_t cmdline_list = LIST_EMPTY;

static void cmdline_put(const char *key, const char *value)
{
  cmdline_pair_t *pair = seq_alloc(sizeof(*pair));
  if (!pair)
    panic("allocating cmdline pair failed");

  pair->key = key;
  pair->value = value;

  list_add_tail(&cmdline_list, &pair->node);
}

// TODO: more error checking for malformed command line
void cmdline_init(multiboot_t *multiboot)
{
  multiboot_tag_t *tag = multiboot_get(multiboot, MULTIBOOT_TAG_CMDLINE);
  if (tag)
  {
    const char *cmdline = tag->cmdline.string;
    if (!(*cmdline))
      return;

    char *key = 0;

    const char *token_start = cmdline;
    for (;;)
    {
      char c = *cmdline;
      if (c == 0 || c == ' ' || c == '=')
      {
        if (token_start == cmdline)
          panic("malformed kernel command line");

        size_t token_len = cmdline - token_start;
        char *token = seq_alloc(token_len + 1);
        if (!token)
          panic("allocating cmdline token failed");

        memcpy(token, token_start, token_len);
        token[token_len] = 0;

        token_start = cmdline + 1;

        if (key == 0)
        {
          if (c == ' ')
            cmdline_put(token, "");
          else
            key = token;
        }
        else
        {
          cmdline_put(key, token);
          key = 0;
        }
      }

      if (c == 0)
        break;

      cmdline++;
    }
  }
}

const char *cmdline_get(const char *key)
{
  list_for_each(&cmdline_list, node)
  {
    cmdline_pair_t *pair = container_of(node, cmdline_pair_t, node);
    if (strcmp(key, pair->key) == 0)
      return pair->value;
  }

  return 0;
}
