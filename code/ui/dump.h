#pragma once

/*
Interacts with the kernel to dump and retrieve information about:
    - Physical memory state
*/

/* INCLUDES */

#include <stdint.h>


/* TYPES */

// Dump types.
typedef enum
{
	// State of the physical memory manager.
	DUMP_PMM_STATE = 0,
	
} dump_type_t;


/* DECLARATIONS */

// Collects the required information and writes it into the given file.
void create_dump(dump_type_t type, const char *filePath);