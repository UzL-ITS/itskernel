#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <io/keycodes.h>

// The possible message destinations.
typedef enum
{
	// The kernel's UI process.
	MSG_DEST_UI_PROCESS = 1,
	
	// The process which display context is currently rendered.
	MSG_DEST_VISIBLE_PROCESS = 2,
} msg_dest_t;

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

// A key press message.
typedef struct
{
	// Message header data.
	msg_header_t header;
	
	// Shift modifier.
	bool shiftModifier;
	
	// The code of the pressed key.
	vkey_t keyCode;
} msg_key_press_t;

// Sends a message to the given process.
void msg_send(msg_dest_t dest, msg_header_t *msg);

// Creates a new key press message.
msg_header_t *msg_create_keypress(vkey_t keyCode, bool shiftModifier);

// Frees the memory of the given message.
void msg_free(msg_header_t *msg);