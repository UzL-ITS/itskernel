
#ifndef _SMP_MODE_H
#define _SMP_MODE_H

typedef enum
{
  MODE_UP, // Uni-processor mode
  MODE_SMP, // Active symmetric multi-processor mode
  MODE_SMP_STARTING // Starting symmetric multi-processor mode
} smp_mode_t;

extern smp_mode_t smp_mode;

#endif
