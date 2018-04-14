#include "types.h"
#include "user.h"
#include "vga.h"

enum splittype{
  SPLIT_VERTICAL,   // one left, one right
  SPLIT_HORIZONTAL, // one above, one below
  SPLIT_NONE,
  SPLIT_SEPARATOR,  // a separator is technically a pane itself
};

#define BORDERTHICKNESS 2 // border is 2px on each side

struct paneholder{
  int child1dim; // is width in vertical split
                 // is height in horizontal split

  struct paneholder *parent;

  struct paneholder *child1;
  struct paneholder *child2;

  struct paneholder *separator;

  enum splittype split;

  uchar *display; // if this is not 0, do not split
  int pid;
};

static struct paneholder *rootpane;
static struct paneholder *activepane;
static int activepanex = 0;
static int activepaney = 0;
static int activepanew = VGA_SCREEN_WIDTH;
static int activepaneh = VGA_SCREEN_HEIGHT;

void
pane_visual_select(int x, int y, int w, int h, int unselect)
{
  uchar color = unselect ? 0xd7 : 0x18; // white or blue

  draw(x, y, &color, 1, w, BORDERTHICKNESS, 1); // top border
  draw(x, y, &color, 1, BORDERTHICKNESS, h, 1); // left border
  draw(x + w - BORDERTHICKNESS, y, &color, 1, BORDERTHICKNESS, h, 1); // right border
  draw(x, y + h - BORDERTHICKNESS, &color, 1, w, BORDERTHICKNESS, 1); // bottom border
}

void
set_active_pane(int x, int y)
{
  struct paneholder *currpane = rootpane;
  int currpanex = 0;
  int currpaney = 0;
  int currpanew = VGA_SCREEN_WIDTH;
  int currpaneh = VGA_SCREEN_HEIGHT;

  while(currpane->split != SPLIT_NONE){
    if(currpane->split == SPLIT_HORIZONTAL){ // one above, one below
      if(y < currpaney + currpane->child1dim){
        currpaneh = currpane->child1dim;
        currpane = currpane->child1;
      }
      else{
        currpaney += currpane->child1dim;
        currpaneh -= currpane->child1dim;
        currpane = currpane->child2;
      }
    }
    else if(currpane->split == SPLIT_VERTICAL){ // one left, one right
      if(x < currpanex + currpane->child1dim){
        currpanew = currpane->child1dim;
        currpane = currpane->child1;
      }
      else{
        currpanex += currpane->child1dim;
        currpanew -= currpane->child1dim;
        currpane = currpane->child2;
      }
    }
    else{
      printf(1, "Error! Bad pane\n");
    }
  }

  if(currpane != activepane){
    pane_visual_select(activepanex, activepaney, activepanew, activepaneh, 1); // unselect prev
    
    activepane = currpane;

    activepanex = currpanex;
    activepaney = currpaney;
    activepanew = currpanew;
    activepaneh = currpaneh;
    pane_visual_select(activepanex, activepaney, activepanew, activepaneh, 0); // select curr
  }
}

// revrows is 1 if rows are given in reverse order, as in a BMP
void
draw_bmp(struct paneholder *window, int col, int row, const uint *bmp_buf, uint w, uint h, int revrows)
{
  if(window->split != SPLIT_NONE)
    return;

  // Offset for given window
  struct paneholder *currpane = window;

  while(currpane != rootpane){
    if(currpane == currpane->parent->child2){
      if(currpane->parent->split == SPLIT_VERTICAL)
        col += currpane->parent->child1dim;
      else if(currpane->parent->split == SPLIT_HORIZONTAL)
        row += currpane->parent->child1dim;
    }
    currpane = currpane->parent;
  }
  // end offset

  int coloffset = 0;
  int rowoffset = 0;

  int startrow = revrows ? h - 1 : 0;
  int endrow = revrows ? -1 : h;

  if(col > VGA_SCREEN_WIDTH || row > VGA_SCREEN_HEIGHT)
    return;

  if(col < 0)
    coloffset = -col;
  if(row < 0)
    rowoffset = -row;
  // Convert regular BMP BGRA data to our custom-indexed web-safe palette
  for(int ri = 0, r = startrow;
      r != endrow
      && r + rowoffset < h && row + ri + rowoffset < VGA_SCREEN_HEIGHT
      && r + rowoffset >= 0 && row + ri + rowoffset >= 0;
      ri++, r = revrows ? r - 1 : r + 1){
    for(int ci = 0; ci + coloffset < w && col + ci + coloffset < VGA_SCREEN_WIDTH; ci++){
      window->display[OFFSET(row + ri, col + ci, VGA_SCREEN_WIDTH)] = BGRATOC(bmp_buf[OFFSET(r, ci, w)]);
    }
  }

//  draw(col, row, window->display, w, h, 1);
//  redraw(col, row, w, h);
}

void
splitpane(void)
{
  activepane->child1 = malloc(sizeof(struct paneholder));
  activepane->child2 = malloc(sizeof(struct paneholder));
  activepane->separator = malloc(sizeof(struct paneholder));

  activepane->child1->split = activepane->child2->split = SPLIT_NONE;
  activepane->separator->split = SPLIT_SEPARATOR;

  activepane->child1->parent = activepane->child2->parent = activepane->separator->parent = activepane;

  activepane->child1->display = activepane->display;
  activepane->child1->pid = activepane->pid;

  activepane->display = (void*)0;
  activepane->pid = -1;

  pane_visual_select(activepanex, activepaney, activepanew, activepaneh, 1); // unselect orig

  activepane = activepane->child1;
}

void
vertsplit(void)
{
  activepane->split = SPLIT_VERTICAL;
  activepane->child1dim = activepanew / 2;
  splitpane();
  activepanew = activepane->parent->child1dim; // same as /= 2

  pane_visual_select(activepanex, activepaney, activepanew, activepaneh, 0); // select curr
  pane_visual_select(activepanex + activepanew, activepaney, activepanew, activepaneh, 1); // unselect other split ch
}

void
horizsplit(void)
{
  activepane->split = SPLIT_HORIZONTAL;
  activepane->child1dim = activepaneh / 2;
  splitpane();
  activepaneh = activepane->parent->child1dim; // same as /= 2

  pane_visual_select(activepanex, activepaney, activepanew, activepaneh, 0); // select curr
  pane_visual_select(activepanex, activepaney + activepaneh, activepanew, activepaneh, 1); // unselect other split ch
}


void
eventloop(void)
{
  int mousex = 0;
  int mousey = 0;

  while(1){
    int event;

    do{
      event = getuserevent();
    }while(event == -1);

    if(event & (1 << 31)){ // cursor moved
      event = event & ~(1 << 31);

      mousey = event & 0xFFFF;
      mousex = (event >> 16) & 0xFFFF;
    }
    else if(event & (1 << 30)){ // mouse click
      if(event & (1 << 2)){ // left btn down
        set_active_pane(mousex, mousey);
      }
    }
    else if(event & (1 << 29)){ // keyboard key press
      char key = event & 0xFF;
      int released = event & (1 << 10);
      //int shiftheld = event & (1 << 9);
      int ctlheld = event & (1 << 8);

      if(!released && ctlheld && key == '\'') // horizontal
        horizsplit();
      else if(!released && ctlheld && key == '/') // vertical
        vertsplit();
    }

//    printf(1, "Event: %d\n", event);
  }
}

int
main(void)
{
  char *argv[] = { "sh", 0 };
  int pid, wpid;

  printf(1, "Starting up the Y window system server...\n");

  printf(1, "init: starting sh\n");
  pid = fork();
  if(pid < 0){
    printf(1, "init: fork failed\n");
    exit();
  }
  if(pid == 0){
    exec("sh", argv);
    printf(1, "init: exec sh failed\n");
    exit();
  }

  struct paneholder startpane;
  startpane.split = SPLIT_NONE;
  startpane.display = (void*)0;

  rootpane = activepane = &startpane;
  pane_visual_select(activepanex, activepaney, activepanew, activepaneh, 0); // select starting pane 
  
  eventloop();
  
  while((wpid=wait()) >= 0 && wpid != pid)
    printf(1, "zombie!\n");
  exit();
}
