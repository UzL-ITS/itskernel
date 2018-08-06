/*
Message passing functionality.
*/

#include <proc/msg.h>
#include <proc/proc.h>
#include <stdlib/stdlib.h>

void msg_send(msg_dest_t dest, msg_header_t *msg)
{
	// Send message
	proc_send_message(dest, msg);
}

msg_header_t *msg_create_keypress(vkey_t keyCode, bool shiftModifier)
{
	// Allocate message memory
	msg_key_press_t *msg = (msg_key_press_t *)malloc(sizeof(msg_key_press_t));
	msg->header.type = MSG_KEY_PRESS;
	msg->header.size = sizeof(msg_key_press_t);
	msg->shiftModifier = shiftModifier;
	msg->keyCode = keyCode;
	return &msg->header;
}

void msg_free(msg_header_t *msg)
{
	free(msg);
}