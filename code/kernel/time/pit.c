
#include <time/pit.h>
#include <bus/isa.h>
#include <cpu/pause.h>
#include <cpu/port.h>
#include <panic/panic.h>
#include <stdint.h>
#include <stdbool.h>
#include <lock/raw_spinlock.h>
#include <trace/trace.h>

/* the frequency of the PIT */
#define PIT_FREQ 1193182

/* I/O ports for controlling the PIT */
#define PORT_CMD  0x0043
#define PORT_CH0  0x0040
#define PORT_CH1  0x0041
#define PORT_CH2  0x0042
#define PORT_CTRL 0x0061 /* NMI status and control port */

/* control port flags */
#define CTRL_GATE    0x01 /* gate pin for CH2 */
#define CTRL_SPEAKER 0x02 /* speaker enable */
#define CTRL_OUT     0x20 /* output pin for CH2 */

/* command port flags */
#define CMD_BINARY    0x00
#define CMD_BCD       0x01
#define CMD_MODE0     0x00 /* interrupt on terminal count */
#define CMD_MODE1     0x08 /* hardware re-triggerable one-shot */
#define CMD_MODE2     0x04 /* rate generator */
#define CMD_MODE3     0x0C /* square wave generator */
#define CMD_MODE4     0x02 /* software triggered strobe */
#define CMD_MODE5     0x0A /* hardware triggered strobe */
#define CMD_ACC_LATCH 0x00
#define CMD_ACC_LO    0x20
#define CMD_ACC_HI    0x10
#define CMD_ACC_LOHI  0x30
#define CMD_CH0       0x00
#define CMD_CH1       0x80
#define CMD_CH2       0x40
#define CMD_READBACK  0xC0

/* readback flags, lowest 6 bits are the same as the CMD bits */
#define RB_NULL 0x40
#define RB_OUT  0x80

static int mdelayCounter = 0;
static raw_spinlock_t mdelayLock = RAW_SPINLOCK_UNLOCKED;

void pit_timer_install_handler(intr_handler_t handler)
{
	/* set the interrupt handler, PIT channel 0 is connected to ISA IRQ0 */
	if (!intr_route_irq(isa_irq(0), handler))
		panic("failed to route ISA IRQ0");
}

void pit_monotonic(int ms)
{
  /* program channel 0 */
  uint16_t count = PIT_FREQ * ms / 100;
  outb_p(PORT_CMD, CMD_BINARY | CMD_MODE2 | CMD_ACC_LOHI | CMD_CH0);
  outb_p(PORT_CH0, count & 0xFF);
  outb_p(PORT_CH0, (count >> 8) & 0xFF);
}

static void _pit_mdelay_handler(cpu_state_t *state)
{
	// Decrement counter
	//trace_printf("#");
	--mdelayCounter;
}

// TODO workaround for QEMU receiving an unhandled interrupt after a few usages of this function
// Usually the handler should be installed and then uninstalled again after timer completion
// Maybe a race condition due to QEMUs emulation, if an PIT interrupt is pending but not yet communicated?
static intr_handler_t mdelayHandler = 0;

void pit_mdelay(int ms)
{
	// Prevent simultaneous access to the PIT
	raw_spinlock_lock(&mdelayLock);
	
	// Install interrupt handler
	if(mdelayHandler == 0)
	{
		mdelayHandler = &_pit_mdelay_handler;
		if(!intr_route_irq(isa_irq(0), &_pit_mdelay_handler))
			panic("Failed to route PIT interrupt");
	}
	
	// Split delay into 5ms chunks, else the PIT count register might overflow
	int num5MsChunks = ms / 5;
	if(num5MsChunks > 0)
	{
		// Reset counter
		mdelayCounter = num5MsChunks;
		
		// Program channel 0 to the given delay
		uint16_t count = PIT_FREQ * 5 / 1000;
		outb_p(PORT_CMD, CMD_BINARY | CMD_MODE2 | CMD_ACC_LOHI | CMD_CH0);
		outb_p(PORT_CH0, count & 0xFF);
		outb_p(PORT_CH0, (count >> 8) & 0xFF);
		
		// Wait
		while(mdelayCounter > 0)
			pause_once();
		
		// Disable PIT again
		outb_p(PORT_CMD, CMD_BINARY | CMD_MODE1 | CMD_ACC_LOHI | CMD_CH0);
	}
	
	// If necessary, wait remaining time
	if(5 * num5MsChunks < ms)
	{
		// Reset counter
		mdelayCounter = 1;
		
		// Program channel 0 to the given delay
		uint16_t count = PIT_FREQ * (ms - 5 * num5MsChunks) / 1000;
		outb_p(PORT_CMD, CMD_BINARY | CMD_MODE2 | CMD_ACC_LOHI | CMD_CH0);
		outb_p(PORT_CH0, count & 0xFF);
		outb_p(PORT_CH0, (count >> 8) & 0xFF);
		
		// Wait
		while(mdelayCounter > 0)
			pause_once();
		
		// Disable PIT again
		outb_p(PORT_CMD, CMD_BINARY | CMD_MODE1 | CMD_ACC_LOHI | CMD_CH0);
	}

	// Uninstall interrupt handler
	//intr_unroute_irq(isa_irq(0), &_pit_mdelay_handler);
	
	// Unlock PIT again
	raw_spinlock_unlock(&mdelayLock);
	
	//trace_printf("pit_mdelay end\n");
}
