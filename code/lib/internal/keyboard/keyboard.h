#pragma once

/*
ITS kernel keyboard handling.
*/

/* INCLUDES */

#include <internal/keyboard/keycodes.h>
#include <stdbool.h>


/* DECLARATIONS */

// Initialize the keyboard (starts the receiver thread).
// Called internally.
void keyboard_init();

// Returns the key code of the next key press.
// If the optional parameter shiftPressed is valid (not 0), the value of the shift modifier key is stored in there.
// This call is blocking.
vkey_t receive_keypress(bool *shiftPressed);

// Returns whether the given key code can be converted into a printable character.
bool key_is_printable_character(vkey_t keyCode);

// Returns whether the given key code corresponds to a navigation key.
bool key_is_navigation_key(vkey_t keyCode);

// Converts the given key code into a printable character.
char key_to_character(vkey_t keyCode, bool shiftPressed);