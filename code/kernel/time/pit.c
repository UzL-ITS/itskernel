
#include <time/pit.h>
#include <bus/isa.h>
#include <cpu/pause.h>
#include <cpu/port.h>
#include <panic/panic.h>
#include <stdint.h>

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

void pit_monotonic(int ms, intr_handler_t handler)
{
  /* set the interrupt handler, PIT channel 0 is connected to ISA IRQ0 */
  if (!intr_route_irq(isa_irq(0), handler))
    panic("failed to route ISA IRQ0");

  /* program channel 0 */
  uint16_t count = PIT_FREQ * ms / 100;
  outb_p(PORT_CMD, CMD_BINARY | CMD_MODE2 | CMD_ACC_LOHI | CMD_CH0);
  outb_p(PORT_CH0, count & 0xFF);
  outb_p(PORT_CH0, (count >> 8) & 0xFF);
}

static void _pit_mdelay(int ms)
{
  /* disable the CH2 GATE pin while we program the PIT */
  uint8_t ctrl = inb_p(PORT_CTRL);
  ctrl &= ~CTRL_GATE;
  outb_p(PORT_CTRL, ctrl);

  /* send the command byte */
  outb_p(PORT_CMD, CMD_BINARY | CMD_MODE0 | CMD_ACC_LOHI | CMD_CH2);

  /* send the count word */
  uint16_t count = PIT_FREQ * ms / 1000;
  outb_p(PORT_CH2, count & 0xFF);
  outb_p(PORT_CH2, (count >> 8) & 0xFF);

  /* enable the CH2 GATE pin and ensure the speaker is disabled */
  ctrl |= CTRL_GATE;
  ctrl &= ~CTRL_SPEAKER;
  outb_p(PORT_CTRL, ctrl);

  /* busy-wait for the countdown to reach zero */
  while ((inb_p(PORT_CTRL) & CTRL_OUT) == 0)
  {
    /* also check the count ourselves */
    outb_p(PORT_CMD, CMD_ACC_LATCH | CMD_CH2);
    uint8_t lo = inb_p(PORT_CH2);
    uint8_t hi = inb_p(PORT_CH2);
    uint16_t count = (hi << 8) | lo;
    if (count == 0)
      break;

    /* don't burn the CPU! */
    pause_once();
  }
}

void pit_mdelay(int ms)
{
  /*
   * as the PIT can only count down from 65535, delays of over ~50ms overflow
   * the count register, so they are split up into multiple 50ms delays here
   */
  while (ms > 50)
  {
    _pit_mdelay(50);
    ms -= 50;
  }

  /* delay for any remaining time */
  if (ms > 0)
    _pit_mdelay(ms);
}
