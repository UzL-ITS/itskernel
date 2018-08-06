#pragma once

/*
ITS kernel messaging definitions.
*/

#include <stdbool.h>
#include <stdint.h>
#include <internal/keyboard/keycodes.h>

// The different message types.
typedef enum
{
	// Invalid message (used for notifying the user process that there is no message available).
	MSG_INVALID = 0,
	
	MSG_KEY_PRESS = 1,
} msg_type_t;

// Defines the header that is included in every message.
typedef struct
{
	// Message type.
	msg_type_t type;
	
	// Full message length (including header).
	uint32_t size;
} msg_header_t;

typedef struct
{
	// Message header data.
	msg_header_t header;
	
	// Shift modifier.
	bool shiftModifier;
	
	// The code of the pressed key.
	vkey_t keyCode;
} msg_key_press_t;