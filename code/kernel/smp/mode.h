
#ifndef _SMP_MODE_H
#define _SMP_MODE_H

typedef enum
{
  MODE_UP, /* uni-processor mode */
  MODE_SMP /* symmetric multi-processor mode */
} smp_mode_t;

extern smp_mode_t smp_mode;

#endif
