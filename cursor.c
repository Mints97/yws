#include "types.h"
#include "vga.h"
#include "defs.h"
#include "spinlock.h"

#define CURSOR_WIDTH 12
#define CURSOR_HEIGHT 18

static const unsigned char cursor[CURSOR_WIDTH * CURSOR_HEIGHT] = {
  BLACK, BLACK, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE,
  BLACK, WHITE, BLACK, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE,
  BLACK, WHITE, WHITE, BLACK, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE,
  BLACK, WHITE, WHITE, WHITE, BLACK, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE,
  BLACK, WHITE, WHITE, WHITE, WHITE, BLACK, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE,
  BLACK, WHITE, WHITE, WHITE, WHITE, WHITE, BLACK, WHITE, WHITE, WHITE, WHITE, WHITE,
  BLACK, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, BLACK, WHITE, WHITE, WHITE, WHITE,
  BLACK, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, BLACK, WHITE, WHITE, WHITE,
  BLACK, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, BLACK, WHITE, WHITE,
  BLACK, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, BLACK, WHITE,
  BLACK, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, BLACK,
  BLACK, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, BLACK, BLACK, BLACK, BLACK, BLACK,
  BLACK, WHITE, WHITE, WHITE, BLACK, WHITE, WHITE, BLACK, WHITE, WHITE, WHITE, WHITE,
  BLACK, WHITE, WHITE, BLACK, WHITE, BLACK, WHITE, WHITE, BLACK, WHITE, WHITE, WHITE,
  BLACK, WHITE, BLACK, WHITE, WHITE, BLACK, WHITE, WHITE, BLACK, WHITE, WHITE, WHITE,
  BLACK, BLACK, WHITE, WHITE, WHITE, WHITE, BLACK, WHITE, WHITE, BLACK, WHITE, WHITE,
  WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, BLACK, WHITE, WHITE, BLACK, WHITE, WHITE,
  WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, BLACK, BLACK, WHITE, WHITE, WHITE,
};

static const unsigned char cursor_mask[CURSOR_WIDTH * CURSOR_HEIGHT] = {
  1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
  1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
  1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
  1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,
  1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
  1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 0, 0,
  1, 1, 1, 0, 0, 1, 1, 1, 1, 0, 0, 0,
  1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0,
  0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0,
};

struct spinlock cursorlock;

void
cursor_action(int lbtn, int rbtn, int mbtn, int dx, int dy, int realevent)
{
  static short mousex = 0;
  static short mousey = 0;

  acquire(&cursorlock);

  dy *= -1; // invert so that y is counted from top

  redraw(mousex, mousey, CURSOR_WIDTH, CURSOR_HEIGHT, 1);

  mousex += dx;
  mousey += dy;

  if(mousex < 0)
    mousex = 0;
  if(mousey < 0)
    mousey = 0;

  if(mousex >= VGA_SCREEN_WIDTH)
    mousex = VGA_SCREEN_WIDTH - 1;
  if(mousey >= VGA_SCREEN_HEIGHT)
    mousey = VGA_SCREEN_HEIGHT - 1;

  if(realevent){
    while(enq_cursormove(mousex, mousey) < 0); // hang until userspace can receive event
    while(enq_cursorclick(lbtn, rbtn, mbtn) < 0);
  }

  draw_masked(mousex, mousey, cursor, cursor_mask, CURSOR_WIDTH, CURSOR_HEIGHT, DRAWDEST_VIDEOMEM, 1);

  release(&cursorlock);
}

void
cursorinit(void)
{
  mouseinit();
  initlock(&cursorlock, "cursor");
}
