#include "types.h"
#include "x86.h"
#include "defs.h"
#include "vga.h"
#include "memlayout.h"

#define VGAMEM ((unsigned char*)P2V(0xA0000))

// TODO: make a lock in vgainit and sync everything up to prevent race
// conditions between CPUs
static unsigned char *vgamem = (unsigned char*)VGAMEM;
static unsigned char vgamainlayer[VGA_SCREEN_WIDTH * VGA_SCREEN_HEIGHT];

void
vgainit(void)
{
  // We use web-safe colors, which have 6 possible shades:
  // 0, 51, 102, 153, 204, 255 (increments of 51).
  // This yields 216 colors.
  // We also have to scale them down for the 6-bit VGA representation.
  //
  // Converting to web-safe color from general RGB
  // (from https://stackoverflow.com/a/13058697/3079266):
  // closest to an RGB component c in range 0-255 is 51 * ((c + 25) / 51)
  //
  // Indexing web-safe colors:
  // For web-safe RGB comonents r, g, b, we can just have (r/51)*36 + (b/51)*6 + g/51
  // This will be within the range 0-216
  outb(0x3C8, 0);
  for(char ri = 0; ri < NREDVALS; ri++){
    for(char gi = 0; gi < NGREENVALS; gi++){
      for(char bi = 0; bi < NBLUEVALS; bi++){
        outb(0x03C9, (ri * 51) >> 2);
        outb(0x03C9, (gi * 51) >> 2);
        outb(0x03C9, (bi * 51) >> 2);
      }
    }
  }
}

void
redraw(int col, int row, uint w, uint h)
{
  int coloffset = 0;
  int rowoffset = 0;

  if(col > VGA_SCREEN_WIDTH || row > VGA_SCREEN_HEIGHT)
    return;

  if(col < 0)
    coloffset = -col;
  if(row < 0)
    rowoffset = -row;

  if(col + coloffset + w >= VGA_SCREEN_WIDTH)
    w = VGA_SCREEN_WIDTH - col - coloffset;

  for(int r = row; r + rowoffset < row + rowoffset + h && r + rowoffset < VGA_SCREEN_HEIGHT; r++)
    if(col + coloffset < VGA_SCREEN_WIDTH)
      memmove(vgamem + OFFSET(r + rowoffset, col + coloffset, VGA_SCREEN_WIDTH),
          vgamainlayer + OFFSET(r + rowoffset, col + coloffset, VGA_SCREEN_WIDTH),
          w);
}

void
draw(int col, int row, const unsigned char *buf, uint w, uint h, int drawtomain)
{
  unsigned char *target = drawtomain ? vgamem : vgamainlayer;

  if(col > VGA_SCREEN_WIDTH || row > VGA_SCREEN_HEIGHT)
    return;

  if(col + w <= VGA_SCREEN_WIDTH && row + h <= VGA_SCREEN_HEIGHT)
    memmove(target + OFFSET(row, col, VGA_SCREEN_WIDTH), buf, w * h);
  else
    draw_masked(col, row, buf, (void*)0, w, h, drawtomain);
}

void
draw_masked(int col, int row, const unsigned char *buf, const unsigned char *mask, uint w, uint h, int drawtomain)
{
  int coloffset = 0;
  int rowoffset = 0;

  unsigned char *target = drawtomain ? vgamem : vgamainlayer;

  if(col > VGA_SCREEN_WIDTH || row > VGA_SCREEN_HEIGHT)
    return;

  if(col < 0)
    coloffset = -col;
  if(row < 0)
    rowoffset = -row;

  //if(col + coloffset + w >= VGA_SCREEN_WIDTH)
  //  w = VGA_SCREEN_WIDTH - col - coloffset;

  for(int ri = 0; ri + rowoffset < h && row + ri + rowoffset < VGA_SCREEN_HEIGHT; ri++){
    for(int ci = 0; ci + coloffset < w && col + ci + coloffset < VGA_SCREEN_WIDTH; ci++){
      if(mask && mask[OFFSET(ri + rowoffset, ci + coloffset, w)]){
        target[OFFSET(row + ri + rowoffset, col + ci + coloffset, VGA_SCREEN_WIDTH)] = buf[OFFSET(ri + rowoffset, ci + coloffset, w)];
      }
    }
  }
}



// revrows is 1 if rows are given in reverse order, as in a BMP
// TODO: fix for going out-of-bounds like in draw and draw_masked
void
draw_rgb(int x, int y, const uint *rgb_buf, uint w, uint h, int revrows)
{
  cprintf("mask = %x\n", inb(0x3C6));

  int startrow = revrows ? h - 1 : 0;
  int endrow = revrows ? -1 : h;

  // Convert regular RGB data to our custom-indexed web-safe palette
  for(int ri = 0, r = startrow; r != endrow; ri++, r = revrows ? r - 1 : r + 1){
    for(int ci = 0; ci < w; ci++){
      if(x + ci <= VGA_SCREEN_WIDTH && y + ri <= VGA_SCREEN_HEIGHT){
        vgamainlayer[OFFSET(y + ri, x + ci, VGA_SCREEN_WIDTH)] = BGRATOC(rgb_buf[OFFSET(r, ci, w)]);
      }
    }
  }
}
