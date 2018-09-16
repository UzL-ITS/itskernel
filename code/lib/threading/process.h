#pragma once

/*
ITS kernel standard library process functionality.
*/

/* INCLUDES */

#include <stdint.h>
#include <stdbool.h>


/* TYPES */


/* DECLARATIONS */

// Starts the given ELF program as a new process.
bool start_process(uint8_t *program, int programLength);