
#ifndef _CPU_FLAGS_H
#define _CPU_FLAGS_H

#include <stdint.h>

#define FLAGS_CF    0x00000001 /* carry */
#define FLAGS_PF    0x00000004 /* parity */
#define FLAGS_AF    0x00000010 /* auxiliary carry */
#define FLAGS_ZF    0x00000040 /* zero */
#define FLAGS_SF    0x00000080 /* sign */
#define FLAGS_TF    0x00000100 /* trap */
#define FLAGS_IF    0x00000200 /* interrupt */
#define FLAGS_DF    0x00000400 /* direction */
#define FLAGS_OF    0x00000800 /* overflow */
#define FLAGS_IOPL0 0x00000000 /* supervisor mode */
#define FLAGS_IOPL3 0x00003000 /* user mode */
#define FLAGS_NT    0x00004000 /* nested task */
#define FLAGS_RF    0x00010000 /* resume */
#define FLAGS_VM    0x00020000 /* vm86 mode */
#define FLAGS_AC    0x00040000 /* alignment check */
#define FLAGS_VIF   0x00080000 /* virtual interrupt */
#define FLAGS_VIP   0x00100000 /* virtual interrupt pending */
#define FLAGS_ID    0x00200000 /* cpuid */

uint64_t flags_read(void);
void flags_write(uint64_t flags);

#endif
