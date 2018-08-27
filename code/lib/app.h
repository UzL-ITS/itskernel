#pragma once

/*
ITS kernel standard library application management.
*/

/* INCLUDES */


/* DECLARATIONS */

// Application startup function. Initializes internal library variables.
void _start();

// Application exit function. Frees library resources and sends the given return code to the OS.
void _end(int exitCode);