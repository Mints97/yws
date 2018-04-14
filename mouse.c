#include "types.h"
#include "x86.h"
#include "defs.h"
#include "traps.h"

#define MOUSE_CMDPORT 0x64
#define MOUSE_DATAPORT 0x60 

#define WAIT_TIME 100000

void
mouse_wait_busybit(void)
{
  /*uint time_out = WAIT_TIME;

  // Read port MOUSE_CMDPORT; keep reading until bit 1 (busy bit) is zero
  while(time_out--){ // Signal
    if(!(inb(MOUSE_CMDPORT) & 2))
      return;
  }*/

  while((inb(MOUSE_CMDPORT) & 2));
  return;
}

void
mouse_wait_dataready(void)
{
  /*uint time_out = WAIT_TIME;

  // loop on reading port 0x64 until bit 0 is one
  while(time_out--){
    if(inb(MOUSE_CMDPORT) & 1)
      return;
  }*/

  while(!(inb(MOUSE_CMDPORT) & 1));
  return;
}

void
mouse_write(char val)
{
  //Wait to be able to send a command
  mouse_wait_busybit();
  //Tell the mouse we are sending a command
  outb(MOUSE_CMDPORT, 0xD4);
  //Wait for the final part
  mouse_wait_busybit();
  //Finally write
  outb(MOUSE_DATAPORT, val);
}

char
mouse_read(void)
{
  mouse_wait_dataready();
  return inb(MOUSE_DATAPORT);
}

void
mouse_handler(void (*mouse_event)(int lbtn, int rbtn, int mbtn, int dx, int dy, int realevent))
{
  static int ninterrupt = 0;
  static char flags, dx;

#define FLAGSBIT(b) (flags & (1 << (b))) 
#define SEXT_INT_MASK ((~((int)0)) ^ 0xFF) // 3 high bytes of int filled with 1-s

  /* Flags:
   * Bit 0 = left button status
   * Bit 1 = right button status
   * Bit 2 = middle button status
   * Bit 3 = 1
   * Bit 4 = change in x, high (9th) bit
   * Bit 5 = change in y, high (9th) bit
   * Bit 6 = x overflowed (flag)
   * Bit 7 = y overflowed (flag)
   */

  mouse_wait_dataready();

  if(!(inb(MOUSE_CMDPORT) & (1 << 5)))
    return;

  if(ninterrupt == 0)
    flags = mouse_read();
  else if(ninterrupt == 1)
    dx = mouse_read();
  else if(ninterrupt == 2){
    char dy = mouse_read();

    mouse_event(!!FLAGSBIT(0), !!FLAGSBIT(1), FLAGSBIT(2), 
        dx | (FLAGSBIT(4) ? SEXT_INT_MASK : 0),
        dy | (FLAGSBIT(5) ? SEXT_INT_MASK : 0),
        1);

    /*if(FLAGSBIT(6))
      cprintf("dx overflowed!\n");
    if(FLAGSBIT(7))
      cprintf("dy overflowed!\n");*/
  }

#undef FLAGSBIT
#undef SEXT_INT_MASK

  ninterrupt = (ninterrupt + 1) % 3; // 3 interrupts
}

void
mouseinit(void)
{ // from http://webcache.googleusercontent.com/search?q=cache:https://www.ssucet.org/old/pluginfile.php/526/mod_folder/content/0/08-keyboardmouse.pdf?forcedownload=1
  // and https://forum.osdev.org/viewtopic.php?t=10247
  
  //Enable the auxiliary mouse device
  mouse_wait_busybit();
  outb(MOUSE_CMDPORT, 0xA8); // This turns on the mouse input
                    // Also clears the “disable mouse” bit in the command byte register
  
  //Enable the interrupts
  mouse_wait_busybit();
  outb(MOUSE_CMDPORT, 0x20);
  mouse_wait_dataready();
  char status = inb(MOUSE_DATAPORT) | 2;
//  outb(MOUSE_CMDPORT, status);
  mouse_wait_busybit();
  outb(MOUSE_CMDPORT, MOUSE_DATAPORT);
  mouse_wait_busybit();
  outb(MOUSE_DATAPORT, status);
  
  //Tell the mouse to use default settings
  mouse_write(0xF6); // send reset command
  char ack = mouse_read();  // Acknowledge

  if(!ack)
    panic("Mouse did not send ack!");
  
  //Enable the mouse
  mouse_write(0xF4); // enable stream mode
  char read = mouse_read();  // Acknowledge

  if(!read)
    panic("Mouse enable failed!");

  // Actually listen to the interrupts!
  picenable(IRQ_MOUSE);
  ioapicenable(IRQ_MOUSE, 0);
}
