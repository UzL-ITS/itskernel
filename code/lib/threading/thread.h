#pragma once

/*
ITS kernel standard library threading functionality.
*/

/* INCLUDES */



/* TYPES */

// Function signature of a thread function.
typedef void (*thread_func_t)(void *funcArgsPtr);


/* DECLARATIONS */

// Initializes the library's threading module.
// Is internally called exactly once immediately on startup.
// This function does not have any side effects (outputs etc.).
void threading_init();

// Runs the given function in a new thread.
void run_thread(thread_func_t funcPtr, void *funcArgsPtr, const char *name);

// Sets the core where the current thread shall be executed on.
void set_thread_affinity(int coreId);