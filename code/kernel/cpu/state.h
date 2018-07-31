
#ifndef _CPU_STATE_H
#define _CPU_STATE_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib/stdnoreturn.h>

/* CPU state passed to intr_dispatch() (and various other places) */
typedef struct
{
  /* the register file */
  uint64_t regs[15];

  /* the error code and interrupt id */
  uint64_t id;
  uint64_t error;

  /* these are pushed automatically by the CPU */
  uint64_t rip;
  uint64_t cs;
  uint64_t rflags;
  uint64_t rsp;
  uint64_t ss;
} __attribute__((__packed__)) cpu_state_t;

#define RAX 0
#define RBX 1
#define RCX 2
#define RDX 3
#define RSI 4
#define RDI 5
#define RBP 6
/* RSP is stored in a separate field */
#define R8  7
#define R9  8
#define R10 9
#define R11 10
#define R12 11
#define R13 12
#define R14 13
#define R15 14

#endif
