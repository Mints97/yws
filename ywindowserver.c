#include "types.h"
#include "user.h"
#include "vga.h"
#include "fcntl.h"
#include "param.h"

enum splittype{
  SPLIT_VERTICAL,   // one left, one right
  SPLIT_HORIZONTAL, // one above, one below
  SPLIT_NONE,
  SPLIT_VSEPARATOR,  // a separator is technically a pane itself
  SPLIT_HSEPARATOR,
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
  int pipefromproc;
  int pipetoproc;
};

static struct paneholder *rootpane;

static struct paneholder *activepane = (void*)0;
static int activepanex = 0;
static int activepaney = 0;
static int activepanew = VGA_SCREEN_WIDTH;
static int activepaneh = VGA_SCREEN_HEIGHT;
static int activepane_parentw = VGA_SCREEN_WIDTH;
static int activepane_parenth = VGA_SCREEN_HEIGHT;

static struct paneholder *selectedsep = (void*)0;
static int selectedsep_ch1x = 0;
static int selectedsep_ch1y = 0;
static int selectedsep_ch1w = VGA_SCREEN_WIDTH;
static int selectedsep_ch1h = VGA_SCREEN_HEIGHT;

static int selectedsep_ch2x = 0;
static int selectedsep_ch2y = 0;
static int selectedsep_ch2w = VGA_SCREEN_WIDTH;
static int selectedsep_ch2h = VGA_SCREEN_HEIGHT;

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

  int prevpanew = VGA_SCREEN_WIDTH;
  int prevpaneh = VGA_SCREEN_HEIGHT;

  while(currpane->split != SPLIT_NONE){
    prevpanew = currpanew;
    prevpaneh = currpaneh;

    if(currpane->split == SPLIT_HORIZONTAL){ // one above, one below
      if(y < currpaney + currpane->child1dim - BORDERTHICKNESS){
        currpaneh = currpane->child1dim;
        currpane = currpane->child1;
      }
      else if(y >= currpaney + currpane->child1dim + BORDERTHICKNESS){
        currpaney += currpane->child1dim;
        currpaneh -= currpane->child1dim;
        currpane = currpane->child2;
      }
      else{ // separator
        selectedsep = currpane->separator;

        selectedsep_ch1x = currpanex;
        selectedsep_ch1y = currpaney;
        selectedsep_ch1w = currpanew;
        selectedsep_ch1h = currpane->child1dim;

        selectedsep_ch2x = currpanex;
        selectedsep_ch2y = currpaney + currpane->child1dim;
        selectedsep_ch2w = currpanew;
        selectedsep_ch2h = currpaneh - currpane->child1dim;

        return;
      }
    }
    else if(currpane->split == SPLIT_VERTICAL){ // one left, one right
      if(x < currpanex + currpane->child1dim - BORDERTHICKNESS){
        currpanew = currpane->child1dim;
        currpane = currpane->child1;
      }
      else if(x >= currpanex + currpane->child1dim + BORDERTHICKNESS){
        currpanex += currpane->child1dim;
        currpanew -= currpane->child1dim;
        currpane = currpane->child2;
      }
      else{ // separator
        selectedsep = currpane->separator;

        selectedsep_ch1x = currpanex;
        selectedsep_ch1y = currpaney;
        selectedsep_ch1w = currpane->child1dim;
        selectedsep_ch1h = currpaneh;

        selectedsep_ch2x = currpanex + currpane->child1dim;
        selectedsep_ch2y = currpaney;
        selectedsep_ch2w = currpanew - currpane->child1dim;
        selectedsep_ch2h = currpaneh;

        return;
      }
    }
    else{
      printf(1, "Should not have happened!\n");
    }
  }

  selectedsep = (void*)0;

  if(currpane != activepane){
    if(activepane)
      pane_visual_select(activepanex, activepaney, activepanew, activepaneh, 1); // unselect prev
    
    activepane = currpane;

    activepanex = currpanex;
    activepaney = currpaney;
    activepanew = currpanew;
    activepaneh = currpaneh;
    
    activepane_parentw = prevpanew;
    activepane_parenth = prevpaneh;

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
  /*struct paneholder *currpane = window;

  while(currpane != rootpane){
    if(currpane == currpane->parent->child2){
      if(currpane->parent->split == SPLIT_VERTICAL)
        col += currpane->parent->child1dim;
      else if(currpane->parent->split == SPLIT_HORIZONTAL)
        row += currpane->parent->child1dim;
    }
    currpane = currpane->parent;
  }*/
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

  drawmasked(col, row, window->display, 0, VGA_SCREEN_WIDTH, VGA_SCREEN_HEIGHT, 1);
//  redraw(col, row, w, h);
}

void
splitpane(void)
{
  activepane->child1 = malloc(sizeof(struct paneholder));
  activepane->child2 = malloc(sizeof(struct paneholder));
  activepane->separator = malloc(sizeof(struct paneholder));

  activepane->child1->split = activepane->child2->split = SPLIT_NONE;

  activepane->child1->parent = activepane->child2->parent = activepane->separator->parent = activepane;

  activepane->child1->display = activepane->display;
  activepane->child1->pid = activepane->pid;

  activepane->child1->pipefromproc = activepane->pipefromproc;
  activepane->child1->pipetoproc = activepane->pipetoproc;

  activepane->display = (void*)0;
  activepane->pid = -1;
  activepane->pipefromproc = -1;
  activepane->pipetoproc = -1;

  pane_visual_select(activepanex, activepaney, activepanew, activepaneh, 1); // unselect orig

  activepane = activepane->child1;
}

void
vertsplit(void)
{
  activepane->split = SPLIT_VERTICAL;
  activepane->child1dim = activepanew / 2;
  splitpane();
  activepane->parent->separator->split = SPLIT_VSEPARATOR;
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
  activepane->parent->separator->split = SPLIT_HSEPARATOR;
  activepaneh = activepane->parent->child1dim; // same as /= 2

  pane_visual_select(activepanex, activepaney, activepanew, activepaneh, 0); // select curr
  pane_visual_select(activepanex, activepaney + activepaneh, activepanew, activepaneh, 1); // unselect other split ch
}

void
resize_pane(struct paneholder *pane, int panex, int paney, int panew, int paneh,
    int dimchangex, int dimchangey) // right or bottom border change
{
  int unselect = 1;

  if(dimchangex)
    dimchangey = 0;
  else if(dimchangey)
    dimchangex = 0;

  if(pane == activepane){
    unselect = 0;
    activepanew = panew + dimchangex;
    activepaneh = paneh + dimchangey;
  }

  if(dimchangex){
    if(pane->split == SPLIT_VERTICAL){
      resize_pane(pane->child2, panex + pane->child1dim, paney, panew - pane->child1dim, paneh,
          dimchangex, 0);
    }
    else if(pane->split == SPLIT_HORIZONTAL){
      resize_pane(pane->child1, panex, paney, panew, pane->child1dim,
          dimchangex, 0);
      resize_pane(pane->child2, panex, paney + pane->child1dim, panew, paneh - pane->child1dim,
          dimchangex, 0);
    }
    else if(pane->split == SPLIT_NONE){
      // TODO: resize contained image, if any
      if(pane->display){
      }
      else{
        if(dimchangex > 0){
          uchar color = 0x00; // black bg
          draw(panex + panew - BORDERTHICKNESS, paney, &color, 1, dimchangex, paneh, 1);
        }
      }
    }
  }
  else if(dimchangey){
    if(pane->split == SPLIT_HORIZONTAL){
      resize_pane(pane->child2, panex, paney + pane->child1dim, panew, paneh - pane->child1dim,
          0, dimchangey);
    }
    else if(pane->split == SPLIT_VERTICAL){
      resize_pane(pane->child1, panex, paney, pane->child1dim, paneh,
          0, dimchangey);
      resize_pane(pane->child2, panex + pane->child1dim, paney, panew - pane->child1dim, paneh,
          0, dimchangey);
    }
    else if(pane->split == SPLIT_NONE){
      // TODO: resize contained image, if any
      if(pane->display){
      }
      else{
        if(dimchangey > 0){
          uchar color = 0x00; // black bg
          draw(panex, paney + paneh - BORDERTHICKNESS, &color, 1, panew, dimchangey, 1);
        }
      }
    }
  }

  pane_visual_select(panex, paney, panew + dimchangex, paneh + dimchangey, unselect); // select curr
}

void
move_pane(struct paneholder *pane, int panex, int paney, int panew, int paneh,
    int dimchangex, int dimchangey) // left or top border change, shift up or down
{
  int unselect = 1;

  if(dimchangex)
    dimchangey = 0;
  else if(dimchangey)
    dimchangex = 0;

  if(pane == activepane){
    unselect = 0;

    activepanex = panex + dimchangex;
    activepaney = paney + dimchangey;

    activepanew = panew - dimchangex;
    activepaneh = paneh - dimchangey;
  }


  if(dimchangex){
    if(pane->split == SPLIT_VERTICAL){
      pane->child1dim += dimchangex;
      move_pane(pane->child1, panex, paney, pane->child1dim - dimchangex, paneh,
          dimchangex, 0);
    }
    else if(pane->split == SPLIT_HORIZONTAL){
      move_pane(pane->child1, panex, paney, panew, pane->child1dim,
          dimchangex, 0);
      move_pane(pane->child2, panex, paney + pane->child1dim, panew, paneh - pane->child1dim,
          dimchangex, 0);
    }
    else if(pane->split == SPLIT_NONE){
      // TODO: move contained image, if any
      if(pane->display){
      }
      else{
        if(dimchangex < 0){
          uchar color = 0x00; // black bg
          draw(panex + dimchangex, paney, &color, 1, -dimchangex + BORDERTHICKNESS, paneh, 1);
        }
      }
    }
  }
  else if(dimchangey){
    if(pane->split == SPLIT_HORIZONTAL){
      pane->child1dim += dimchangey;
      move_pane(pane->child1, panex, paney, panew, pane->child1dim - dimchangey,
          0, dimchangey);
    }
    else if(pane->split == SPLIT_VERTICAL){
      move_pane(pane->child1, panex, paney, pane->child1dim, paneh,
          0, dimchangey);
      move_pane(pane->child2, panex + pane->child1dim, paney, panew - pane->child1dim, paneh,
          0, dimchangey);
    }
    else if(pane->split == SPLIT_NONE){
      // TODO: move contained image, if any
      if(pane->display){
      }
      else{
        if(dimchangey < 0){
          uchar color = 0x00; // black bg
          draw(panex, paney + dimchangey, &color, 1, panew, -dimchangey + BORDERTHICKNESS, 1);
        }
      }
    }
  }

  pane_visual_select(panex + dimchangex, paney + dimchangey, panew - dimchangex, paneh - dimchangey, unselect); // select curr
}


void
destroy_activepane(void)
{
  if(activepane == rootpane)
    return;

  if(activepane->pid != -1){
    kill(activepane->pid);
  }

  struct paneholder *activepane_parent = activepane->parent;
  struct paneholder *otherchild; 

  if(activepane == activepane_parent->child2){
    otherchild = activepane_parent->child1;

    if(activepane_parent->split == SPLIT_VERTICAL){
      int otherchildw = activepane_parent->child1dim;
      resize_pane(otherchild, activepanex - otherchildw, activepaney, otherchildw, activepaneh, activepanew, 0);
    }
    else if(activepane_parent->split == SPLIT_HORIZONTAL){
      int otherchildh = activepane_parent->child1dim;
      resize_pane(otherchild, activepanex, activepaney - otherchildh, activepanew, otherchildh, 0, activepaneh);
    }
  }
  else{
    otherchild = activepane_parent->child2;

    if(activepane_parent->split == SPLIT_VERTICAL){
      int otherchildw = activepane_parentw - activepanew;
      move_pane(otherchild, activepanex + activepanew, activepaney, otherchildw, activepaneh, -activepanew, 0);
    }
    else if(activepane_parent->split == SPLIT_HORIZONTAL){
      int otherchildh = activepane_parenth - activepaneh;
      move_pane(otherchild, activepanex, activepaney + activepaneh, activepanew, otherchildh, 0, -activepaneh);
    }
  }

  // Clone (lul)
  activepane_parent->split = otherchild->split;
  activepane_parent->child1 = otherchild->child1;
  activepane_parent->child2 = otherchild->child2;
  free(activepane_parent->separator);
  activepane_parent->separator = otherchild->separator;
  activepane_parent->child1dim = otherchild->child1dim;
  if(activepane_parent->display)
    free(activepane_parent->display);
  activepane_parent->display = otherchild->display;
  activepane_parent->pid = otherchild->pid;
  activepane_parent->pipefromproc = otherchild->pipefromproc;
  activepane_parent->pipetoproc = otherchild->pipetoproc;

  if(otherchild->split == SPLIT_VERTICAL || otherchild->split == SPLIT_HORIZONTAL){
    otherchild->child1->parent = activepane_parent;
    otherchild->child2->parent = activepane_parent;
  }

  free(activepane);

  activepane = (void*)0;
}

struct paneholder*
find_pane(struct paneholder *currpane, int pipefromproc)
{
  if(currpane->split == SPLIT_VERTICAL || currpane->split == SPLIT_HORIZONTAL){
    struct paneholder *ch1res = find_pane(currpane->child1, pipefromproc);

    if(ch1res)
      return ch1res;

    return find_pane(currpane->child2, pipefromproc);
  }
  else if(currpane->pipefromproc == pipefromproc)
    return currpane;
  else
    return (void*)0;
}

int
make_sh(void)
{
  char *argv[] = { "sh", 0 };

  // assumes at least fds 0, 1, 2 are taken
  int ptoproc[2]; // [1] is write, [0] is read
  pipe(ptoproc);

  int ptoserv[2];
  pipe(ptoserv);

  int pid;

  printf(1, "yws: starting sh\n");
  pid = fork();
  if(pid < 0){
    printf(1, "yws: fork failed\n");
    exit();
  }
  if(pid == 0){
    close(ptoproc[1]);
    close(ptoserv[0]);

    if(ptoproc[0] > 3){
      close(3);
      dup(ptoproc[0]);
      close(ptoproc[0]);
    }

    if(ptoproc[1] > 4){
      close(4);
    }

    dup(ptoserv[1]);

    // end invariant: read from fd 3, write to fd 4
    exec("sh", argv);
    printf(1, "yws: exec sh failed\n");
    exit();
  }

  close(ptoproc[0]);
  close(ptoserv[1]);
  int toserv = dup(ptoserv[0]);
  close(ptoserv[0]);

  return toserv; // end invariant: read from returned fd, write to next fd
}

void
eventloop(void)
{
  int prevmousex = 0;
  int prevmousey = 0;

  int mousex = 0;
  int mousey = 0;

  int proccomm_toserv[NPROC];
  int proccomm_toproc[NPROC];

  int npipes = 1;

  int toserv = make_sh();
  int toproc = toserv + 1;

  proccomm_toserv[0] = toserv;
  proccomm_toproc[0] = toproc;

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
        if(selectedsep){ // we've clicked a separator and haven't released yet
          if(selectedsep->split == SPLIT_VSEPARATOR){
            int dx = mousex - prevmousex;
            resize_pane(selectedsep->parent->child1, selectedsep_ch1x, selectedsep_ch1y, selectedsep_ch1w, selectedsep_ch1h,
                dx, 0);
            move_pane(selectedsep->parent->child2, selectedsep_ch2x, selectedsep_ch2y, selectedsep_ch2w, selectedsep_ch2h,
                dx, 0);

            selectedsep_ch1w += dx;
            selectedsep_ch2w -= dx;
            selectedsep->parent->child1dim += dx;
            selectedsep_ch2x += dx;

            pane_visual_select(activepanex, activepaney, activepanew, activepaneh, 0); // select active pane again
          }
          else if(selectedsep->split == SPLIT_HSEPARATOR){
            int dy = mousey - prevmousey;
            resize_pane(selectedsep->parent->child1, selectedsep_ch1x, selectedsep_ch1y, selectedsep_ch1w, selectedsep_ch1h,
                0, dy);
            move_pane(selectedsep->parent->child2, selectedsep_ch2x, selectedsep_ch2y, selectedsep_ch2w, selectedsep_ch2h,
                0, dy);

            selectedsep_ch1h += dy;
            selectedsep_ch2h -= dy;
            selectedsep->parent->child1dim += dy;
            selectedsep_ch2y += dy;

            pane_visual_select(activepanex, activepaney, activepanew, activepaneh, 0); // select active pane again
          }
        }
        else{
          set_active_pane(mousex, mousey);
        }
      }
      else{ // left button up
        selectedsep = (void*)0;
      }

      prevmousex = mousex;
      prevmousey = mousey;
    }
    else if(event & (1 << 29)){ // keyboard key press
      char key = event & 0xFF;
      int released = event & (1 << 10);
      //int shiftheld = event & (1 << 9);
      int ctlheld = event & (1 << 8);

      if(activepane){
        if(!released && ctlheld){
          if(key == '\''){ // horizontal
            horizsplit();
          }
          else if(key == '/'){ // vertical
            vertsplit();
          }
          else if(key == 'd'){ // close pane
            destroy_activepane();
          }
          else if(key == 's'){ // make new shell
            toserv = make_sh();
            toproc = toserv + 1;

            proccomm_toserv[npipes] = toserv;
            proccomm_toproc[npipes] = toproc;
            npipes++;
          }
        }
      }
    }

    for(int i = 0; i < npipes; i++){
      char buf[100];

      if(getpipesize(proccomm_toserv[i]) > 0 && read(proccomm_toserv[i], buf, 100) > 0){
        struct paneholder *targetpane = find_pane(rootpane, proccomm_toserv[i]);

        if(!targetpane){
          targetpane = activepane;
          if(activepane->display){
            free(activepane->display);
          }
          //activepane->display = malloc((activepanew - BORDERTHICKNESS * 2) * (activepaneh - BORDERTHICKNESS * 2));
          activepane->display = malloc(VGA_SCREEN_WIDTH * VGA_SCREEN_HEIGHT);


          // TODO: if a proc was running in activepane, close it!
        }

        if(buf[7] == ' ')
          buf[7] = '\0';
        if(strcmp(buf, "drawbmp") == 0){
          int fd = open(buf + 8, O_RDONLY);

          if(fd < 0){
            printf(proccomm_toproc[i], "nack");
            continue;
          }

          char imgheader[0x36] = {0};

          read(fd, imgheader, 0x36);

          uint w = ((uint*)(imgheader + 0x12))[0];
          uint h = ((uint*)(imgheader + 0x16))[0];

          uint *imgdata = malloc(w * h * 4);

          if(read(fd, imgdata, w * h * 4) < 0){
            printf(proccomm_toproc[i], "nack");
            continue;
          }

          draw_bmp(targetpane, 0, 0, imgdata, w, h, 1);

          close(fd);
          free(imgdata);
        }

        printf(proccomm_toproc[i], "ack");
      }
    }
  }
}

int
main(void)
{
  int fd;

  // Ensure that three file descriptors are open.
  while((fd = open("console", O_RDWR)) >= 0){
    if(fd >= 3){
      close(fd);
      break;
    }
  }

  int wpid;

  printf(1, "Starting up the Y window system server...\n");

  struct paneholder startpane;
  startpane.split = SPLIT_NONE;
  startpane.display = (void*)0;
  startpane.pid = -1;
  startpane.pipetoproc = -1;
  startpane.pipefromproc = -1;

  rootpane = activepane = &startpane;
  pane_visual_select(activepanex, activepaney, activepanew, activepaneh, 0); // select starting pane 
  
  eventloop();
  
  while((wpid = wait()) >= 0 && wpid != getpid())
    printf(1, "zombie!\n");
  exit();
}
