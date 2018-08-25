#pragma once

/*
ITS kernel raw mutexes (used as base for higher-level locking).
*/

/* INCLUDES */

#include <stdint.h>


/* TYPES */

// Type of a raw mutex.
typedef uint64_t raw_mutex_t;


/* DECLARATIONS */

// Initializes a new raw mutex.
void raw_mutex_init(raw_mutex_t *mutex);

// Acquires the given raw mutex; if it is already acquired by another thread, this function blocks.
void raw_mutex_acquire(raw_mutex_t *mutex);

// Releases the given raw mutex.
void raw_mutex_release(raw_mutex_t *mutex);