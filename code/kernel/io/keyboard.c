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
#include <cpu/pause.h>
#include <time/pit.h>

// PS/2 controller constants.
#define PS2_DATA_PORT 0x60
#define PS2_CONFIG_PORT 0x64
#define PS2_STATUS_PORT 0x64


// Currently pressed keys.
static bool pressedKeys[VKEY_MAX_VALUE + 1] = { false };

// Code E0 in last interrupt?
static bool receivedE0 = false;

// Determines whether the polling workaround is active.
static bool pollWorkaround = false;


static void _handle_key_press(cpu_state_t *UNUSED_state)
{
	// Retrieve scan code
	uint8_t scanCode = inb(PS2_DATA_PORT);
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

static void _output_wait()
{
	// Wait for data available status bit
	uint8_t status;
	while(!((status = inb(PS2_STATUS_PORT)) & 0x01))
		pause_once();
	trace_printf("Leaving _output_wait with status %02x\n", status);
}

static void _input_wait()
{
	// Wait until controller is ready for input
	uint8_t status;
	while(1)
	{
		// Input buffer empty and controller ready?
		status = inb(PS2_STATUS_PORT);
		if(!(status & 0x02) && (status & 0x04))
			break;
		
		// Wait a moment
		pause_once();
	}
	trace_printf("Leaving _input_wait with status %02x\n", status);
}

void keyboard_poll()
{
	// Polling enabled?
	if(!pollWorkaround)
		return;
	
	// Call interrupt handler, while new data is available
//	while(inb(PS2_STATUS_PORT) & 0x01)
		_handle_key_press(0);
}

void keyboard_init()
{
	trace_printf("Initializing keyboard...\n");
	
	// Do self-test
	outb(PS2_CONFIG_PORT, 0xAA);
	pit_mdelay(2);
	_output_wait();
	trace_printf("8042 self-test result: %02x\n", inb(PS2_DATA_PORT));
	outb(PS2_CONFIG_PORT, 0xAB);
	pit_mdelay(2);
	_output_wait();
	trace_printf("8042 Port 1 self-test result: %02x\n", inb(PS2_DATA_PORT));
	
	// Discard pending output bytes
	while(inb(PS2_STATUS_PORT) & 0x01)
		inb(PS2_DATA_PORT);
	
	// Read configuration byte
	outb(PS2_CONFIG_PORT, 0x20);
	pit_mdelay(2);
	_output_wait();
	uint8_t configByte = inb(PS2_DATA_PORT);
	trace_printf("Configuration byte: %02x\n", configByte);
	
	// TODO workaround: If the configuration is not immediately correct, do not enable interrupt at all and use polling instead
	if(!(configByte & 0x03))
	{
		// Enable workaround
		trace_printf("Enabling keyboard workaround...\n");
		pollWorkaround = true;
		
		// Update configuration register
		outb(PS2_CONFIG_PORT, 0x60);
		pit_mdelay(2);
		_input_wait();
		outb(PS2_DATA_PORT, 0x64); // Enable port 1, disable IRQs
		pit_mdelay(2);
		
		
		outb(PS2_CONFIG_PORT, 0x20);
		pit_mdelay(2);
		_output_wait();
		trace_printf("New configuration byte: %02x\n", inb(PS2_DATA_PORT));
		
		//TODO: Bei neuem NUC wird Config-Byte einmal mit +0x01 erhöht (bei steigender flanke), bei altem gar nicht
		//-> Polling sollte dort gar nicht funktionieren
		//TEST
		/*pit_mdelay(2);
		while(1)
		{
			trace_printf("%02x %02x\n", inb(PS2_STATUS_PORT), inb(PS2_DATA_PORT));
			pit_mdelay(500);
		}
		halt_forever();*/
	}
	else
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
	
	// Discard pending output bytes
	pit_mdelay(2);
	while(inb(PS2_STATUS_PORT) & 0x01)
	{
		inb(PS2_DATA_PORT);
		pit_mdelay(2);
	}
	
	trace_printf("Keyboard initialization end\n");
}