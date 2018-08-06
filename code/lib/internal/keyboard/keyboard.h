#pragma once

/*
ITS kernel keyboard handling.
*/

/* INCLUDES */

#include <internal/keyboard/keycodes.h>
#include <stdbool.h>


/* DECLARATIONS */

// Returns the key code of the next key press.
// If the optional parameter shiftPressed is valid (not 0), the value of the shift modifier key is stored in there.
// This call is blocking.
vkey_t receive_keypress(bool *shiftPressed);

// Returns whether the given key code can be converted into a printable character.
bool key_is_printable_character(vkey_t keyCode);

// Converts the given key code into a printable character.
char key_to_character(vkey_t keyCode, bool shiftPressed);