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
static bool pressedKeys[128] = { false };

static void _handle_key_press(cpu_state_t *state)
{
	// TODO move code to other places as much as possible, since the whole core gets blocked while handling this interrupt
	
	// Retrieve scan code
	// TODO add support for extended scan codes (E0/E1)
	uint8_t scanCode = inb(0x60);
	trace_printf("Scan code 0x%02x\n", scanCode);
	
	// Key release?
	if(scanCode & 0x80)
	{
		pressedKeys[scanCode & 0x7F] = false;
	}
	else
	{
		// Key not pressed yet?
		if(!pressedKeys[scanCode])
		{
			// Mark key as pressed
			pressedKeys[scanCode] = true;
	
			// Create message
			msg_header_t *keypressMsg = msg_create_keypress(scanCode);
	
			// Send Fx keys always to the UI process
			if((KEYBOARD_SCAN_CODE_F1 <= scanCode && scanCode <= KEYBOARD_SCAN_CODE_F10) || scanCode == KEYBOARD_SCAN_CODE_F11 || scanCode == KEYBOARD_SCAN_CODE_F12)
				msg_send(MSG_DEST_UI_PROCESS, keypressMsg);
			else
				msg_send(MSG_DEST_VISIBLE_PROCESS, keypressMsg);
		}
	}
}

void keyboard_init()
{
	// TODO geht einfach nicht...
	//     Debug-outputs einfügen in _intr_route_irq
	// Install interrupt
	irq_tuple_t tuple;
	tuple.irq = 1;
	tuple.active_polarity = POLARITY_HIGH;
	tuple.trigger = TRIGGER_EDGE;
	if(!intr_route_irq(&tuple, _handle_key_press))
		trace_puts("Error: Could not install keyboard interrupt handler.\n");
	else
		trace_puts("Keyboard interrupt handler successfully installed.\n");
	
	
	// - inportb for scan code
	// - Console switching with Fx keys
	// - No LEDs for now
	// - Message pipe for current process window -> how? Whole message system per process needed
}