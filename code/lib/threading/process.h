#pragma once

/*
ITS kernel standard library process functionality.
*/

/* INCLUDES */

#include <stdint.h>
#include <stdbool.h>


/* TYPES */


/* DECLARATIONS */

// Runs the given executable in a new process.
bool start_process(const char *programPath);