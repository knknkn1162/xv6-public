// Intel 8250 serial port (UART).
// This defines a serial port (UART type 16550A). see dot-bachsrc.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "traps.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"

#define COM1    0x3f8

static int uart;    // is there a uart?

void
uartinit(void)
{
  char *p;

  // Turn off the FIFO
  // Writing a "0" to bit 0 will disable the FIFOs, in essence turning the UART into 8250 compatibility mode.
  outb(COM1+2, 0);

  // 9600 baud, 8 data bits, 1 stop bit, parity off.
  // 0b10000000
  // Bit7: 1.. Setting the Divisor Latch Access Bit (DLAB), allowing you to set the values of the Divisor Latch Bytes.
  // Bit5,4,3..000 -> No Parity
  // Bit2.. One Stop Bit
  outb(COM1+3, 0x80);    // Unlock divisor
  // divisor latch low byte: 0x0c
  outb(COM1+0, 115200/9600);
  outb(COM1+1, 0);
  // Bit1,2.. 11 -> word length is 8 bits
  // Bit7 .. 0 -> Lock divisor
  outb(COM1+3, 0x03);    // Lock divisor, 8 data bits.
  // This register allows you to do "hardware" flow control, under software control.
  outb(COM1+4, 0);
  // bit0: Enable Received Data Available Interrupt
  outb(COM1+1, 0x01);    // Enable receive interrupts.

  // If status is 0xFF, no serial port.
  if(inb(COM1+5) == 0xFF)
    return;
  uart = 1;

  // Acknowledge pre-existing interrupt conditions;
  // enable interrupts.
  inb(COM1+2);
  inb(COM1+0);
  // usually, COM1 <-> IRQ4(#define IRQ_COM1         4)
  ioapicenable(IRQ_COM1, 0);

  // Announce that we're here.
  for(p="xv6...\n"; *p; p++)
    uartputc(*p);
}

void
uartputc(int c)
{
  int i;

  if(!uart)
    return;
  // #define COM1    0x3f8
  // Bits 5 and 6 refer to the condition of the character transmitter circuits and can help you to identify if the UART is ready to accept another character.
  // i: retry count
  for(i = 0; i < 128 && !(inb(COM1+5) & 0x20); i++)
    microdelay(10);
  // Transmitter Holding Buffer. Now the data is sent!!
  outb(COM1+0, c);
}

static int
uartgetc(void)
{
  if(!uart)
    return -1;
  if(!(inb(COM1+5) & 0x01))
    return -1;
  return inb(COM1+0);
}

void
uartintr(void)
{
  consoleintr(uartgetc);
}
