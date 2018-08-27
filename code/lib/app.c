/*
ITS kernel standard library application management.
*/

/* INCLUDES */

#include <app.h>
#include <io.h>
#include <threading/thread.h>


/* VARIABLES */


/* FUNCTIONS */

void _start()
{
	// Initialize threading
	threading_init();
	
	// Create terminal
	io_init(1000);
}

void _end(int exitCode)
{
	// TODO clean exit
	while(1);
}