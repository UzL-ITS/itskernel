
#ifndef _CPU_FEATURES_H
#define _CPU_FEATURES_H

#include <stdbool.h>

typedef enum
{
  FEATURE_1G_PAGE,
  _FEATURE_MAX
} cpu_feature_t;

void cpu_features_init(void);
bool cpu_feature_supported(cpu_feature_t feature);

#endif
