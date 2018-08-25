#pragma once

/*
ITS kernel standard library locking functions.
Currently supported:
	- Mutexes

The functions here all expect pointers to user-allocated memory for the objects.
*/

/* INCLUDES */

#include <internal/mutex/raw.h>


/* TYPES */

// Type of a mutex.
typedef raw_mutex_t mutex_t;


/* DECLARATIONS */

// Initializes the given mutex.
void mutex_init(mutex_t *mutex);

// Acquires the given raw mutex; if it is already acquired by another thread, this function blocks.
void mutex_acquire(mutex_t *mutex);

// Releases the given raw mutex.
void mutex_release(mutex_t *mutex);