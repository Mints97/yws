#include "types.h"
#include "x86.h"
#include "defs.h"
#include "kbd.h"

// Moved from kbdgetc
// Will be used elsewhere!
static int
applycaps(uint c, uint shift)
{
  if(shift & CAPSLOCK){
    if('a' <= c && c <= 'z')
      c += 'A' - 'a';
    else if('A' <= c && c <= 'Z')
      c += 'a' - 'A';
  }
  return c;
}

int
kbdgetc(void)
{
  static uint shift;
  static uchar *charcode[4] = {
    normalmap, shiftmap, ctlmap, ctlmap
  };
  uint st, data, c;

  st = inb(KBSTATP);
  if((st & KBS_DIB) == 0 // available
      || (st & (1 << 5))) // data from mouse!
    return -1;
  data = inb(KBDATAP);

  if(data == 0xE0){
    shift |= E0ESC;
    return 0;
  } else if(data & 0x80){
    // Key released
    data = (shift & E0ESC ? data : data & 0x7F);
    shift &= ~(shiftcode[data] | E0ESC);

    // Decodes the keyboard event, and puts it on the queue
    // for Y to read
    uchar evc = charcode[shift & SHIFT][data];
    while(enq_kbevent(evc, 1, shift & SHIFT, shift & CTL) < 0); // hang
    return 0;
  } else if(shift & E0ESC){
    // Last character was an E0 escape; or with 0x80
    data |= 0x80;
    shift &= ~E0ESC;
  }

  shift |= shiftcode[data];
  shift ^= togglecode[data];


  uchar evc = charcode[shift & SHIFT][data];
  while(enq_kbevent(evc, 0, shift & SHIFT, shift & CTL) < 0); // hang


  c = charcode[shift & (CTL | SHIFT)][data];
  return applycaps(c, shift);
}

void
kbdintr(void)
{
  consoleintr(kbdgetc);
}
