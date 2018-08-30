#pragma once

/*
ITS kernel LWIP port configuration definitions.
See doc/sys_arch.txt for further explanations.
*/

/* INCLUDES */

#include <memory.h>
#include <io.h>


/* OTHER */

// sprintf() formatters
#define LWIP_NO_INTTYPES_H 1
#define U16_F "hu"
#define S16_F "hd"
#define X16_F "hx"
#define U32_F "u"
#define S32_F "d"
#define X32_F "x"

// We currently do not have an errno.h file
#define LWIP_PROVIDE_ERRNO

// Byte order functions
#define BYTE_ORDER LITTLE_ENDIAN
#define LWIP_PLATFORM_BYTESWAP 1
#define LWIP_PLATFORM_HTONS(x) ( (((u16_t)(x))>>8) | (((x)&0xFF)<<8) )
#define LWIP_PLATFORM_HTONL(x) ( (((u32_t)(x))>>24) | (((x)&0xFF0000)>>8) | (((x)&0xFF00)<<8) | (((x)&0xFF)<<24) )

// Packing macros
#define PACK_STRUCT_FIELD(x) x
#define PACK_STRUCT_STRUCT  __attribute__((packed))
#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_END

// Debug outputs
#define LWIP_PLATFORM_DIAG(x) do { \
        printf(x);                 \
    } while(0)

#define LWIP_PLATFORM_ASSERT(x) do {                                              \
        printf("Assertion failed at line %d in %s: %s\n", __LINE__, __FILE__, x); \
    } while(0)
