
#include <cpu/features.h>
#include <cpu/cpuid.h>
#include <stdlib/string.h>
#include <stddef.h>
#include <stdint.h>
#include <limits.h>

#define BITS_PER_ELEMENT (sizeof(uint64_t) * CHAR_BIT)
#define ELEMENTS ((_FEATURE_MAX + BITS_PER_ELEMENT - 1) / BITS_PER_ELEMENT)

uint64_t features[ELEMENTS];

static void cpu_feature_set(cpu_feature_t feature)
{
  int element = feature / BITS_PER_ELEMENT;
  int bit = feature % BITS_PER_ELEMENT;
  features[element] |= (0x1 << bit);
}

void cpu_features_init(void)
{
  /* reset all feature flags */
  memset(features, 0, sizeof(features));

  /* find max cpuid functions supported */
  uint32_t max, max_ext, tmp;
  cpu_id(CPUID_VENDOR, &max, &tmp, &tmp, &tmp);
  cpu_id(CPUID_EXT_VENDOR, &max_ext, &tmp, &tmp, &tmp);

  /* detect 1GB page support */
  if (CPUID_EXT_FEATURES <= max_ext)
  {
    uint32_t edx;
    cpu_id(CPUID_EXT_FEATURES, &tmp, &tmp, &tmp, &edx);
    if (edx & CPUID_EXT_FEATURE_EDX_1GB_PAGE)
      cpu_feature_set(FEATURE_1G_PAGE);
  }
}

bool cpu_feature_supported(cpu_feature_t feature)
{
  int element = feature / BITS_PER_ELEMENT;
  int bit = feature % BITS_PER_ELEMENT;
  return (features[element] >> bit) & 0x1;
}
