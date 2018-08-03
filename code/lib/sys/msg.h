#pragma once

#include <stdint.h>

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
	
	// The scan code of the pressed key.
	// TODO use more reliable virtual key codes here
	uint8_t scanCode;
} msg_key_press_t;