#include "types.h"
#include "x86.h"
#include "vga.h"
#include "defs.h"
#include "memlayout.h"
#include "spinlock.h"

#define VGAMEM ((unsigned char*)P2V(0xA0000))

static unsigned char *vgamem = (unsigned char*)VGAMEM;
static unsigned char vgamainlayer[VGA_SCREEN_WIDTH * VGA_SCREEN_HEIGHT];

static struct spinlock vgalock;

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

  initlock(&vgalock, "vga");
}

void
redraw(int col, int row, uint w, uint h, int fromcursor)
{
  int coloffset = 0;
  int rowoffset = 0;

  if(col > VGA_SCREEN_WIDTH || row > VGA_SCREEN_HEIGHT)
    return;

  if(col < 0)
    coloffset = -col;
  if(row < 0)
    rowoffset = -row;

  if(col + coloffset + w > VGA_SCREEN_WIDTH)
    w = VGA_SCREEN_WIDTH - (col + coloffset);

  acquire(&vgalock);
  for(int ri = 0; ri + rowoffset < h && row + ri + rowoffset < VGA_SCREEN_HEIGHT; ri++){
    int offset = OFFSET(row + ri + rowoffset, col + coloffset, VGA_SCREEN_WIDTH);
    memmove(vgamem + offset, vgamainlayer + offset, w);
  }
  release(&vgalock);

  if(!fromcursor)
    cursor_action(0, 0, 0, 0, 0, 0); // we might have redrawn over the cursor
}

void
draw(int col, int row, const uchar *buf, int onecolor, uint dimw, uint w, uint h, enum drawdest dest, int fromcursor)
{
  int coloffset = 0;
  int rowoffset = 0;

  uchar *target = dest == DRAWDEST_VIDEOMEM ? vgamem : vgamainlayer;

  if(col > VGA_SCREEN_WIDTH || row > VGA_SCREEN_HEIGHT)
    return;

  if(col < 0)
    coloffset = -col;
  if(row < 0)
    rowoffset = -row;

  int width_toscr = VGA_SCREEN_WIDTH - (col + coloffset);
  int width_fromimg = w - coloffset;
  int width = width_toscr < width_fromimg ? width_toscr : width_fromimg;

  acquire(&vgalock);
  for(int ri = 0; ri + rowoffset < h && row + ri + rowoffset < VGA_SCREEN_HEIGHT; ri++){
    uint target_offset = OFFSET(row + ri + rowoffset, col + coloffset, VGA_SCREEN_WIDTH);

    if(onecolor){
      memset(target + target_offset, *buf, width);
      if(dest == DRAWDEST_BOTH)
        memset(vgamem + target_offset, *buf, width);
    }
    else{
      const uchar *data_source = buf + OFFSET(ri + rowoffset, coloffset, dimw);
     
      memmove(target + target_offset, data_source, width);
      if(dest == DRAWDEST_BOTH)
        memmove(vgamem + target_offset, data_source, width);
    }
  }
  release(&vgalock);

  if(dest != DRAWDEST_MAINLAYER && !fromcursor)
    cursor_action(0, 0, 0, 0, 0, 0); // we might have redrawn over the cursor
}

void
draw_masked(int col, int row, const unsigned char *buf, const unsigned char *mask, uint w, uint h, enum drawdest dest, int fromcursor)
{
  int coloffset = 0;
  int rowoffset = 0;

  unsigned char *target = dest == DRAWDEST_VIDEOMEM ? vgamem : vgamainlayer;

  if(col > VGA_SCREEN_WIDTH || row > VGA_SCREEN_HEIGHT)
    return;

  if(col < 0)
    coloffset = -col;
  if(row < 0)
    rowoffset = -row;

  acquire(&vgalock);
  for(int ri = 0; ri + rowoffset < h && row + ri + rowoffset < VGA_SCREEN_HEIGHT; ri++){
    for(int ci = 0; ci + coloffset < w && col + ci + coloffset < VGA_SCREEN_WIDTH; ci++){
      int vga_offset = OFFSET(row + ri + rowoffset, col + ci + coloffset, VGA_SCREEN_WIDTH);
      int buf_offset = OFFSET(ri + rowoffset, ci + coloffset, w);
      if(!mask || mask[buf_offset]){
        target[vga_offset] = buf[buf_offset];
        if(dest == DRAWDEST_BOTH)
          vgamem[vga_offset] = buf[buf_offset];
      }
    }
  }
  release(&vgalock);

  if(!fromcursor)
    cursor_action(0, 0, 0, 0, 0, 0); // we might have redrawn over the cursor
}
