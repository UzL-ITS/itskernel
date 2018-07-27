
#ifndef _CPU_EFER_H
#define _CPU_EFER_H

#include <stdint.h>

#define EFER_SCE 0x00000001 /* syscall enable */
#define EFER_LME 0x00000100 /* long mode enable */
#define EFER_LMA 0x00000400 /* long mode active */
#define EFER_NXE 0x00000800 /* nx bit enable */

uint64_t efer_read(void);
void efer_write(uint64_t efer);

#endif
