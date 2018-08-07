/*
Keyboard driver.
*/

#include <io/keyboard.h>
#include <cpu/port.h>
#include <cpu/state.h>
#include <intr/common.h>
#include <intr/route.h>
#include <trace/trace.h>
#include <vbe/vbe.h>
#include <stdbool.h>
#include <proc/msg.h>
#include <proc/proc.h>


// Currently pressed keys.
static bool pressedKeys[VKEY_MAX_VALUE + 1] = { false };

// Code E0 in last interrupt?
static bool receivedE0 = false;

static void _handle_key_press(cpu_state_t *state)
{
	// Retrieve scan code
	// TODO add support for extended scan codes (E0)
	// TODO add support for modifiers -> add fields to messages
	uint8_t scanCode = inb(0x60);
	trace_printf("Scan code 0x%02x\n", scanCode);
	
	// Check scan code type
	vkey_t keyCode = VKEY_INVALID;
	bool pressed = true;
	if(scanCode == 0xE0)
	{
		// Set flag
		receivedE0 = true;
		return;
	}
	else if(receivedE0)
	{
		// Reset flag
		receivedE0 = false;
		
		// Retrieve virtual key code
		keyCode = e0ScanCodeConversionTable[scanCode & 0x7F];
		
		// Key release?
		if(scanCode & 0x80)
			pressed = false;
	}
	else
	{
		// Retrieve virtual key code
		keyCode = oneByteScanCodeConversionTable[scanCode & 0x7F];
		
		// Key release?
		if(scanCode & 0x80)
			pressed = false;
	}
	
	// Send key press message
	if(pressed)
	{
		// Create message
		bool shiftModifier = pressedKeys[VKEY_LSHIFT] || pressedKeys[VKEY_RSHIFT];
		msg_header_t *keypressMsg = msg_create_keypress(keyCode, shiftModifier);

		// Send Fx keys always to the UI process
		if(VKEY_F1 <= keyCode && keyCode <= VKEY_F12)
		{
			// Key not pressed yet?
			if(!pressedKeys[keyCode])
				msg_send(MSG_DEST_UI_PROCESS, keypressMsg);
		}
		else
			msg_send(MSG_DEST_VISIBLE_PROCESS, keypressMsg);
	}
	
	// Update key state
	pressedKeys[keyCode] = pressed;
}

void keyboard_init()
{
	// Install interrupt
	irq_tuple_t tuple;
	tuple.irq = 1;
	tuple.active_polarity = POLARITY_HIGH;
	tuple.trigger = TRIGGER_EDGE;
	if(!intr_route_irq(&tuple, _handle_key_press))
		trace_puts("Error: Could not install keyboard interrupt handler.\n");
	else
		trace_puts("Keyboard interrupt handler successfully installed.\n");
}