
#include <acpi/common.h>

bool acpi_table_valid(acpi_header_t *table)
{
  uint8_t sum = 0;
  uint8_t *ptr_start = (uint8_t *) table;
  uint8_t *ptr_end = ptr_start + table->len;

  for (uint8_t *ptr = ptr_start; ptr < ptr_end; ptr++)
    sum += *ptr;

  return sum == 0;
}
