/*
ITS kernel keyboard handling.
*/

/* INCLUDES */

#include <internal/keyboard/keyboard.h>
#include <internal/syscall/syscalls.h>


/* VARIABLES */


/* FUNCTIONS */

vkey_t receive_keypress(bool *shiftPressed)
{
	// Wait for key press message
	msg_type_t newMsgType;
	while((newMsgType = sys_next_message_type()) == MSG_INVALID)
		sys_yield();
	
	// Retrieve message
	msg_key_press_t msg;
	sys_next_message(&msg.header);
	
	// Shift modifier?
	if(shiftPressed)
		*shiftPressed = msg.shiftModifier;
	
	return msg.keyCode;
}

bool key_is_printable_character(vkey_t keyCode)
{
	return (VKEY_A <= keyCode && keyCode <= VKEY_Z)
		|| (VKEY_0 <= keyCode && keyCode <= VKEY_9)
		|| (keyCode == VKEY_SPACE) || (keyCode == VKEY_TAB)
		|| (VKEY_SEMICOLON <= keyCode && keyCode <= VKEY_RBRACKET)
		|| (VKEY_KPMULTIPLY <= keyCode && keyCode <= VKEY_KP9);
}

bool key_is_navigation_key(vkey_t keyCode)
{
	return (VKEY_CURSORUP <= keyCode && keyCode <= VKEY_END);
}

char key_to_character(vkey_t keyCode, bool shiftPressed)
{
	// Calculate index for key code conversion table and return resulting character
	int index = 2 * keyCode;
	if(shiftPressed)
		++index;
	return keyCodeToAsciiConversionTable[index];
}