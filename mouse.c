#include "types.h"
#include "x86.h"
#include "defs.h"
#include "traps.h"

#define MOUSEPORT 0x64

#define WAIT_TIME 100000

void
mouse_wait_busybit(void)
{
  uint time_out = WAIT_TIME;

  // Read port 0x64; keep reading until bit 1 (busy bit) is zero
  while(time_out--){ // Signal
    if(!(inb(0x64) & 2))
      return;
  }
  return;
}

void
mouse_wait_dataready(void)
{
  uint time_out = WAIT_TIME;

  // loop on reading port 0x64 until bit 0 is one
  while(time_out--){
    if(inb(0x64) & 1)
      return;
  }
  return;
}

void
mouse_write(char val)
{
  //Wait to be able to send a command
  mouse_wait_busybit();
  //Tell the mouse we are sending a command
  outb(0x64, 0xD4);
  //Wait for the final part
  mouse_wait_busybit();
  //Finally write
  outb(0x60, val);
}

char
mouse_read(void)
{
  mouse_wait_dataready();
  return inb(0x60);
}

void
mouse_handler(void (*mouse_event)(int lbtn, int rbtn, int mbtn, int dx, int dy))
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

  if(ninterrupt == 0)
    flags = mouse_read();
  else if(ninterrupt == 1)
    dx = mouse_read();
  else if(ninterrupt == 2){
    char dy = mouse_read();

    mouse_event(!!FLAGSBIT(0), !!FLAGSBIT(1), FLAGSBIT(2), 
        dx | (FLAGSBIT(4) ? SEXT_INT_MASK : 0),
        dy | (FLAGSBIT(5) ? SEXT_INT_MASK : 0));

    /*if(FLAGSBIT(6))
      cprintf("dx overflowed!\n");
    if(FLAGSBIT(7))
      cprintf("dy overflowed!\n");*/
  }

#undef FLAGSBIT

  ninterrupt = (ninterrupt + 1) % 3; // 3 interrupts
}

void
mouseinit(void)
{ // from https://www.ssucet.org/old/pluginfile.php/526/mod_folder/content/0/08-keyboardmouse.pdf?forcedownload=1
  // and https://forum.osdev.org/viewtopic.php?t=10247
  
  //Enable the auxiliary mouse device
  mouse_wait_busybit();
  outb(0x64, 0xA8); // This turns on the mouse input●
                    // Also clears the “disable mouse” bit in the command byte register
  
  //Enable the interrupts
  mouse_wait_busybit();
  outb(0x64, 0x20);
  mouse_wait_dataready();
  char status = inb(0x60) | 2;
  //mouse_wait(1);
  outb(0x64, status);
  mouse_wait_busybit();
  outb(0x64, 0x60);
  mouse_wait_busybit();
  outb(0x60, status);
  
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
