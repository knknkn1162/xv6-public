#include "types.h"
#include "x86.h"
#include "defs.h"
#include "kbd.h"

// from trap -> consoleintr(kbdgetc); -> while((c = kbdgetc()) >= 0) -> kbdgetc
int kbdgetc(void)
{
  static uint shift;
  static uchar *charcode[4] = {
    // no SHIFT and no CTL
    normalmap,
    // SHIFT and no CTL
    shiftmap,
    // no SHIFT and CTL
    ctlmap,
    // SHIFT and CTL
    ctlmap
  };
  uint st, data, c;

  // 0064	r	KB controller read status (ISA, EISA)
  // 		 bit 7 = 1 parity error on transmission from keyboard
  // 		 bit 6 = 1 receive timeout
  // 		 bit 5 = 1 transmit timeout
  // 		 bit 4 = 0 keyboard inhibit
  // 		 bit 3 = 1 data in input register is command
  // 			 0 data in input register is data
  // 		 bit 2	 system flag status: 0=power up or reset  1=selftest OK
  // 		 bit 1 = 1 input buffer full (input 60/64 has data for 8042)
  // 		 bit 0 = 1 output buffer full (output 60 has data for system)
  st = inb(KBSTATP); // #define KBSTATP 0x64 
  // // kbd data in buffer
  if((st & KBS_DIB) == 0) // KBS_DIB=0x01
    return -1;
  data = inb(KBDATAP); // #define KBDATAP 0x60

  if(data == 0xE0) { // 0x1100_0000
    shift |= E0ESC; // #define E0ESC           (1<<6)
    return 0;
  } else if(data & 0x80){
    // Key released
    data = (shift & E0ESC ? data : data & 0x7F);
    shift &= ~(shiftcode[data] | E0ESC);
    return 0;
  } else if(shift & E0ESC){
    // Last character was an E0 escape; or with 0x80
    data |= 0x80;
    shift &= ~E0ESC;
  }

/*
static uchar shiftcode[256] =
{
  [0x1D] CTL,
  [0x2A] SHIFT,
  [0x36] SHIFT,
  [0x38] ALT,
  [0x9D] CTL,
  [0xB8] ALT
};
 */
  shift |= shiftcode[data];
/*
static uchar togglecode[256] =
{
  [0x3A] CAPSLOCK,
  [0x45] NUMLOCK,
  [0x46] SCROLLLOCK
};
 */
  shift ^= togglecode[data];
  // #define SHIFT           (1<<0)
  // #define CTL             (1<<1)
  c = charcode[shift & (CTL | SHIFT)][data];
  if(shift & CAPSLOCK){ // #define CAPSLOCK        (1<<3)
    if('a' <= c && c <= 'z')
      c += 'A' - 'a'; // to uppercase
    else if('A' <= c && c <= 'Z')
      c += 'a' - 'A'; // to lowercase
  }
  return c;
}

void
kbdintr(void)
{
  consoleintr(kbdgetc);
}
