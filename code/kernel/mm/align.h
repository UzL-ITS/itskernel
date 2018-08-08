
#ifndef _MM_ALIGN_H
#define _MM_ALIGN_H

// PAGE_ALIGN: Find the lowest page-aligned address >= x
// PAGE_ALIGN_REVERSE: Find the highest page-aligned address <= x

#define PAGE_ALIGN(x) (((x) + 0xFFF) & 0xFFFFFFFFFFFFF000)
#define PAGE_ALIGN_REVERSE(x) ((x) & 0xFFFFFFFFFFFFF000)

#define PAGE_ALIGN_2M(x) (((x) + 0x1FFFFF) & 0xFFFFFFFFFFE00000)
#define PAGE_ALIGN_REVERSE_2M(x) ((x) & 0xFFFFFFFFFFE00000)

#define PAGE_ALIGN_1G(x) (((x) + 0x3FFFFFFF) & 0xFFFFFFFFC0000000)
#define PAGE_ALIGN_REVERSE_1G(x) ((x) & 0xFFFFFFFFC0000000)

#endif
