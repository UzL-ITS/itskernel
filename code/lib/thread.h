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
void threading_init();

// Runs the given function in a new thread.
void run_thread(thread_func_t funcPtr, void *funcArgsPtr);