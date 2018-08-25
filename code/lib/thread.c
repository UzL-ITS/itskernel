/*
ITS kernel standard library threading functionality.
*/

/* INCLUDES */

#include <thread.h>
#include <lock.h>
#include <internal/syscall/syscalls.h>


/* VARIABLES */

// Mutex to protect parameters on thread creation.
static mutex_t threadCreationMutex;

// Pointer to the function that is used for the thread that is being started at the moment.
static thread_func_t threadFuncPtr;

// Pointer to thread function arguments for the thread that is being started at the moment.
static void *threadFuncArgsPtr;


/* FUNCTIONS */

void threading_init()
{
	// Initialize mutex
	mutex_init(&threadCreationMutex);
}

// Wrapper function to run the thread function and clean up after thread exit.
static void thread_wrapper()
{
	// Thread is up, copy parameters and release the protecting mutex
	thread_func_t funcPtr = threadFuncPtr;
	void *funcArgsPtr = threadFuncArgsPtr;
	mutex_release(&threadCreationMutex);
	
	// Run user function
	funcPtr(funcArgsPtr);
	
	// Delete thread
	sys_exit_thread();
}

void run_thread(thread_func_t funcPtr, void *funcArgsPtr)
{
	// Only one thread can be started at a time
	mutex_acquire(&threadCreationMutex);
	
	// Store wrapper function parameters
	threadFuncPtr = funcPtr;
	threadFuncArgsPtr = funcArgsPtr;
	
	// Run wrapper function in a new thread
	uint64_t wrapperFuncAddress = (uint64_t)&thread_wrapper;
	sys_run_thread(wrapperFuncAddress);
}