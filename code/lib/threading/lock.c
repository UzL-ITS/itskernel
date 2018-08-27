/*
ITS kernel standard library locking functions.
*/

/* INCLUDES */

#include <threading/lock.h>


/* VARIABLES */



/* FUNCTIONS */

void mutex_init(mutex_t *mutex)
{
	// Call corresponding raw mutex function
	raw_mutex_init(mutex);
}

void mutex_acquire(mutex_t *mutex)
{
	// Call corresponding raw mutex function
	raw_mutex_acquire(mutex);
}

void mutex_release(mutex_t *mutex)
{
	// Call corresponding raw mutex function
	raw_mutex_release(mutex);
}