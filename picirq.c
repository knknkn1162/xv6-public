#include "types.h"
#include "x86.h"
#include "traps.h"

// I/O Addresses of the two programmable interrupt controllers
#define IO_PIC1         0x20    // Master (IRQs 0-7)
/*
0021	r/w	PIC master interrupt mask register
		OCW1:
		 bit 7 = 0  enable parallel printer interrupt
		 bit 6 = 0  enable diskette interrupt
		 bit 5 = 0  enable fixed disk interrupt
		 bit 4 = 0  enable serial port 1 interrupt
		 bit 3 = 0  enable serial port 2 interrupt
		 bit 2 = 0  enable video interrupt
		 bit 1 = 0  enable keyboard, mouse, RTC interrupt
		 bit 0 = 0  enable timer interrupt
 */
#define IO_PIC2         0xA0    // Slave (IRQs 8-15)
/*
00A0	r/w	PIC 2  same as 0020 for PIC 1
00A1	r/w	PIC 2  same as 0021 for PIC 1 except for OCW1:
		 bit 7 = 0  reserved
		 bit 6 = 0  enable fixed disk interrupt
		 bit 5 = 0  enable coprocessor exception interrupt
		 bit 4 = 0  enable mouse interrupt
		 bit 3 = 0  reserved
		 bit 2 = 0  reserved
		 bit 1 = 0  enable redirect cascade
		 bit 0 = 0  enable real-time clock interrupt
 */

// Don't use the 8259A interrupt controllers.  Xv6 assumes SMP hardware.
void
picinit(void)
{
  // mask all interrupts
  outb(IO_PIC1+1, 0xFF);
  outb(IO_PIC2+1, 0xFF);
}

//PAGEBREAK!
// Blank page.
