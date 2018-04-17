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

static int ncollapsed = 0; 

void
pane_visual_select(int x, int y, int w, int h, int unselect)
{
  uchar color = unselect ? 0xd7 : 0x18; // white or blue

  draw(x, y, &color, 1, 0, w, BORDERTHICKNESS, 1); // top border
  draw(x, y, &color, 1, 0, BORDERTHICKNESS, h, 1); // left border
  draw(x + w - BORDERTHICKNESS, y, &color, 1, 0, BORDERTHICKNESS, h, 1); // right border
  draw(x, y + h - BORDERTHICKNESS, &color, 1, 0, w, BORDERTHICKNESS, 1); // bottom border
}

void
recalculate_ncollapsed(struct paneholder *currpane, int currpanex, int currpaney, int currpanew, int currpaneh)
{
  if(currpane->split == SPLIT_VERTICAL || currpane->split == SPLIT_HORIZONTAL){
    if(currpane->split == SPLIT_VERTICAL){
      recalculate_ncollapsed(currpane->child1, currpanex, currpaney, currpane->child1dim, currpaneh);
    }
    else{ // HORIZONTAL
      recalculate_ncollapsed(currpane->child1, currpanex, currpaney, currpanew, currpane->child1dim);
    }

    if(currpane->split == SPLIT_VERTICAL){
      recalculate_ncollapsed(currpane->child2, currpanex + currpane->child1dim, currpaney, currpanew - currpane->child1dim, currpaneh);
      return;
    }
    else{ // HORIZONTAL
      recalculate_ncollapsed(currpane->child2, currpanex, currpaney + currpane->child1dim, currpanew, currpaneh - currpane->child1dim);
      return;
    }
  }
  else{
    if(currpanew <= 2 * BORDERTHICKNESS)
      ncollapsed++;
    if(currpaneh <= 2 * BORDERTHICKNESS)
      ncollapsed++;
  }
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

//        find_closest_sep(currpane->child1, currpanex, currpaney, currpanew, currpane->child1dim, selectedsep->split);
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

//        find_closest_sep(currpane->child1, currpanex, currpaney, currpane->child1dim, currpaneh, selectedsep->split);
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
draw_bmp(struct paneholder *window, int nbytesperpx, int col, int row, const uchar *bmp_buf, uint w, uint h, int revrows)
{
  if(nbytesperpx < 3){
    printf(1, "Not 32-bit BGRA or 24-bit BGR, Cannot handle bmp file of this format!\n");
  }

  if(window->split != SPLIT_NONE)
    return;

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
      uint color = *((uint*)(bmp_buf + nbytesperpx * OFFSET(r, ci, w)));
      window->display[OFFSET(row + ri, col + ci, VGA_SCREEN_WIDTH)] = BGRATOC(color);
    }
  }
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
  activepane->child2->display = (void*)0;
  activepane->child2->pid = -1;
  activepane->child2->pipefromproc = -1;
  activepane->child2->pipetoproc = -1;

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
  uchar color = 0x00; // black bg
  draw(activepanex + activepanew / 2 + BORDERTHICKNESS, activepaney, &color, 1, 0,
      activepanew - activepanew / 2 - 2 * BORDERTHICKNESS, activepaneh, 1);
  
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
  uchar color = 0x00; // black bg
  draw(activepanex, activepaney + activepaneh / 2 + BORDERTHICKNESS, &color, 1, 0,
      activepanew, activepaneh - activepaneh / 2 - 2 * BORDERTHICKNESS, 1);

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
      if(dimchangex > 0){
        if(pane->display){
          draw(panex + panew - BORDERTHICKNESS, paney + BORDERTHICKNESS,
              pane->display + OFFSET(0, panew - 2 * BORDERTHICKNESS, VGA_SCREEN_WIDTH),
              0, VGA_SCREEN_WIDTH,
              dimchangex, paneh - 2 * BORDERTHICKNESS, 1);
        }
        else{
          uchar color = 0x00; // black bg
          draw(panex + panew - BORDERTHICKNESS, paney, &color, 1, 0, dimchangex, paneh, 1);
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
      if(dimchangey > 0){
        if(pane->display){
          draw(panex + BORDERTHICKNESS, paney + paneh - BORDERTHICKNESS,
              pane->display + OFFSET(paneh - 2 * BORDERTHICKNESS, 0, VGA_SCREEN_WIDTH),
              0, VGA_SCREEN_WIDTH,
              panew - 2 * BORDERTHICKNESS, dimchangey, 1);
        }
        else{
          uchar color = 0x00; // black bg
          draw(panex, paney + paneh - BORDERTHICKNESS, &color, 1, 0, panew, dimchangey, 1);
        }
      }
    }
  }

  pane_visual_select(panex, paney, panew + dimchangex, paneh + dimchangey, unselect); // select curr
}

void
move_pane(struct paneholder *pane, int panex, int paney, int panew, int paneh,
    int dimchangex, int dimchangey, int compress) // left or top border change, shift up or down
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

    activepanew = compress ? panew - dimchangex : panew;
    activepaneh = compress ? paneh - dimchangey : paneh;
  }

  if(dimchangex){
    if(pane->split == SPLIT_VERTICAL){
      move_pane(pane->child1, panex, paney, pane->child1dim, paneh,
          dimchangex, 0,
          0);
      move_pane(pane->child2, panex + pane->child1dim, paney, panew - pane->child1dim, paneh,
          dimchangex, 0,
          compress);
    }
    else if(pane->split == SPLIT_HORIZONTAL){
      move_pane(pane->child1, panex, paney, panew, pane->child1dim,
          dimchangex, 0,
          1);
      move_pane(pane->child2, panex, paney + pane->child1dim, panew, paneh - pane->child1dim,
          dimchangex, 0,
          1);
    }
    else if(pane->split == SPLIT_NONE){
      if(pane->display){
        draw(panex + dimchangex + BORDERTHICKNESS, paney + BORDERTHICKNESS,
            pane->display, 0, VGA_SCREEN_WIDTH,
            panew - (compress ? dimchangex : 0) - 2 * BORDERTHICKNESS, paneh - 2 * BORDERTHICKNESS, 1);
      }
      else{
        uchar color = 0x00; // black bg
        if(dimchangex < 0){
          draw(panex + dimchangex, paney, &color, 1, 0, -dimchangex + BORDERTHICKNESS, paneh, 1);
        }
        else if(dimchangex > 0 && !compress){
          draw(panex + panew - BORDERTHICKNESS, paney, &color, 1, 0, dimchangex, paneh, 1);
        }
      }
    }
  }
  else if(dimchangey){
    if(pane->split == SPLIT_HORIZONTAL){
      move_pane(pane->child1, panex, paney, panew, pane->child1dim,
          0, dimchangey,
          0);
      move_pane(pane->child2, panex, paney + pane->child1dim, panew, paneh - pane->child1dim,
          0, dimchangey,
          compress);
    }
    else if(pane->split == SPLIT_VERTICAL){
      move_pane(pane->child1, panex, paney, pane->child1dim, paneh,
          0, dimchangey,
          1);
      move_pane(pane->child2, panex + pane->child1dim, paney, panew - pane->child1dim, paneh,
          0, dimchangey,
          1);
    }
    else if(pane->split == SPLIT_NONE){
      if(pane->display){
        draw(panex + BORDERTHICKNESS, paney + dimchangey + BORDERTHICKNESS,
            pane->display, 0, VGA_SCREEN_WIDTH,
            panew - 2 * BORDERTHICKNESS, paneh - (compress ? dimchangey : 0) - 2 * BORDERTHICKNESS, 1);
      }
      else{
        uchar color = 0x00; // black bg
        if(dimchangey < 0){
          draw(panex, paney + dimchangey, &color, 1, 0, panew, -dimchangey + BORDERTHICKNESS, 1);
        }
        else if(dimchangey > 0 && !compress){
          draw(panex, paney + paneh - BORDERTHICKNESS, &color, 1, 0, panew, dimchangey, 1);
        }
      }
    }
  }

  pane_visual_select(panex + dimchangex, paney + dimchangey, compress ? panew - dimchangex : panew, compress ? paneh - dimchangey : paneh, unselect); // select curr
}


void
destroy_activepane(void)
{
  if(activepane == rootpane){
    if(activepane->display){
      free(activepane->display);
      uchar color = 0x00; // black bg
      draw(0, 0, &color, 1, 0, VGA_SCREEN_WIDTH, VGA_SCREEN_HEIGHT, 1);
    }
    activepane->display = (void*)0;
    activepane = (void*)0;
    pane_visual_select(0, 0, VGA_SCREEN_WIDTH, VGA_SCREEN_HEIGHT, 1); // unselect root
    return;
  }

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
      move_pane(otherchild, activepanex + activepanew, activepaney, otherchildw, activepaneh, -activepanew, 0, 1);
    }
    else if(activepane_parent->split == SPLIT_HORIZONTAL){
      int otherchildh = activepane_parenth - activepaneh;
      move_pane(otherchild, activepanex, activepaney + activepaneh, activepanew, otherchildh, 0, -activepaneh, 1);
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

static int targetpanex = 0;
static int targetpaney = 0;
static int targetpanew = VGA_SCREEN_WIDTH;
static int targetpaneh = VGA_SCREEN_HEIGHT;

struct paneholder*
find_pane(struct paneholder *currpane, int currpanex, int currpaney, int currpanew, int currpaneh, int pipefromproc)
{
  if(currpane->split == SPLIT_VERTICAL || currpane->split == SPLIT_HORIZONTAL){
    struct paneholder *ch1res;

    if(currpane->split == SPLIT_VERTICAL){
      ch1res = find_pane(currpane->child1, currpanex, currpaney, currpane->child1dim, currpaneh, pipefromproc);
    }
    else{ // HORIZONTAL
      ch1res = find_pane(currpane->child1, currpanex, currpaney, currpanew, currpane->child1dim, pipefromproc);
    }

    if(ch1res)
      return ch1res;

    if(currpane->split == SPLIT_VERTICAL){
      return find_pane(currpane->child2, currpanex + currpane->child1dim, currpaney, currpanew - currpane->child1dim, currpaneh, pipefromproc);
    }
    else{ // HORIZONTAL
      return find_pane(currpane->child2, currpanex, currpaney + currpane->child1dim, currpanew, currpaneh - currpane->child1dim, pipefromproc);
    }
  }
  else if(currpane->pipefromproc == pipefromproc){
    targetpanex = currpanex;
    targetpaney = currpaney;
    targetpanew = currpanew;
    targetpaneh = currpaneh;
    return currpane;
  }
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

#define BMP_HEADER_START_SIZE 0x36

int
proccomm(int *proccomm_toserv, int *proccomm_toproc, int npipes)
{
  for(int i = 0; i < npipes; i++){
    char buf[100];

    if(getpipesize(proccomm_toserv[i]) > 0){
      int nread = read(proccomm_toserv[i], buf, 100);

      if(nread > 0){
        struct paneholder *targetpane = find_pane(rootpane, 0, 0, VGA_SCREEN_WIDTH, VGA_SCREEN_HEIGHT, proccomm_toserv[i]);

        if(!targetpane){
          targetpane = activepane;
          if(activepane->display){
            free(activepane->display);
          }
          
          activepane->display = malloc(VGA_SCREEN_WIDTH * VGA_SCREEN_HEIGHT);
          memset(activepane->display, 0x00, VGA_SCREEN_WIDTH * VGA_SCREEN_HEIGHT); // set bg color, maybe proc should be able to set it?
          targetpanew = activepanew;
          targetpaneh = activepaneh;
          targetpanex = activepanex;
          targetpaney = activepaney;

          if(activepane->pid != -1)
            kill(activepane->pid);
        }

        buf[nread] = '\0';

        if(buf[7] == ' ')
          buf[7] = '\0';
        if(strcmp(buf, "drawbmp") == 0){
          int fd = open(buf + 8, O_RDONLY);

          if(fd < 0){
            printf(1, "Failed to open file %s\n", buf + 8);
            printf(proccomm_toproc[i], "nack");
            close(fd);
            continue;
          }

          // Using data from http://www.fastgraph.com/help/bmp_header_format.html
          char imgheader[BMP_HEADER_START_SIZE] = {0};

          read(fd, imgheader, BMP_HEADER_START_SIZE);

          uint imgdataoffset = ((uint*)(imgheader + 0xA))[0];

          uint w = ((uint*)(imgheader + 0x12))[0];
          uint h = ((uint*)(imgheader + 0x16))[0];

          uint nbits = ((uint*)(imgheader + 28))[0];

          uint offset = imgdataoffset > BMP_HEADER_START_SIZE ? imgdataoffset - BMP_HEADER_START_SIZE : 0;

          uchar *imgdata = malloc(offset + w * h * 4);
          //printf(1, "reading image...\n");

          if(read(fd, imgdata, offset + w * h * 4) < 0){
            printf(1, "Failed to read file %s\n", buf + 8);
            printf(proccomm_toproc[i], "nack");
            close(fd);
            free(imgdata);
            continue;
          }

          draw_bmp(targetpane, nbits / 8, 0, 0, imgdata + offset, w, h, 1);

          draw(targetpanex + BORDERTHICKNESS, targetpaney + BORDERTHICKNESS, targetpane->display, 0,
              VGA_SCREEN_WIDTH,
              w > targetpanew - 2 * BORDERTHICKNESS ? targetpanew - 2 * BORDERTHICKNESS : w,
              h > targetpaneh - 2 * BORDERTHICKNESS ? targetpaneh - 2 * BORDERTHICKNESS : h, 1);

          close(fd);
          free(imgdata);

          return 0;
        }

        printf(proccomm_toproc[i], "ack");
      }
    }
  }

  return -1;
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
    int proccommres;

    do{
      event = getuserevent();
      proccommres = proccomm(proccomm_toserv, proccomm_toproc, npipes);
    }while(event == -1 && proccommres == -1);

    if(event == -1)
      continue;

    if(event & (1 << 31)){ // cursor moved
      event = event & ~(1 << 31);

      mousey = event & 0xFFFF;
      mousex = (event >> 16) & 0xFFFF;
    }
    else if(event & (1 << 30)){ // mouse click
      if(event & (1 << 2)){ // left btn down
        if(selectedsep){ // we've clicked a separator and haven't released yet
          int ncollapsedprev = ncollapsed;

          if(selectedsep->split == SPLIT_VSEPARATOR){
            int dx = mousex - prevmousex;
            selectedsep->parent->child1dim += dx;
            recalculate_ncollapsed(rootpane, 0, 0, VGA_SCREEN_WIDTH, VGA_SCREEN_HEIGHT);
            selectedsep->parent->child1dim -= dx;

            if(ncollapsed <= ncollapsedprev
                && ((dx < 0 && selectedsep_ch1w > 2 * BORDERTHICKNESS)
                  || (dx > 0 && selectedsep_ch2w > 2 * BORDERTHICKNESS))){
              resize_pane(selectedsep->parent->child1, selectedsep_ch1x, selectedsep_ch1y, selectedsep_ch1w, selectedsep_ch1h,
                  dx, 0);
              move_pane(selectedsep->parent->child2, selectedsep_ch2x, selectedsep_ch2y, selectedsep_ch2w, selectedsep_ch2h,
                  dx, 0,
                  1);

              selectedsep_ch1w += dx;
              selectedsep_ch2w -= dx;
              selectedsep->parent->child1dim += dx;
              selectedsep_ch2x += dx;

              pane_visual_select(activepanex, activepaney, activepanew, activepaneh, 0); // select active pane again
            }
          }
          else if(selectedsep->split == SPLIT_HSEPARATOR){
            int dy = mousey - prevmousey;
            selectedsep->parent->child1dim += dy;
            recalculate_ncollapsed(rootpane, 0, 0, VGA_SCREEN_WIDTH, VGA_SCREEN_HEIGHT);
            selectedsep->parent->child1dim -= dy;
          
            if(ncollapsed <= ncollapsedprev
                && ((dy < 0 && selectedsep_ch1h > 2 * BORDERTHICKNESS)
                  || (dy > 0 && selectedsep_ch2h > 2 * BORDERTHICKNESS))){
              resize_pane(selectedsep->parent->child1, selectedsep_ch1x, selectedsep_ch1y, selectedsep_ch1w, selectedsep_ch1h,
                  0, dy);
              move_pane(selectedsep->parent->child2, selectedsep_ch2x, selectedsep_ch2y, selectedsep_ch2w, selectedsep_ch2h,
                  0, dy,
                  1);

              selectedsep_ch1h += dy;
              selectedsep_ch2h -= dy;
              selectedsep->parent->child1dim += dy;
              selectedsep_ch2y += dy;

              pane_visual_select(activepanex, activepaney, activepanew, activepaneh, 0); // select active pane again
            }
          }
        }
        else{
          set_active_pane(mousex, mousey);
        }
      }
      else{ // left button up
        selectedsep = (void*)0;
        ncollapsed = 0;
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
          else if(key == ','){ // vertical
            vertsplit();
          }
          else if(key == '-'){ // close pane
            destroy_activepane();
          }
          else if(key == '='){ // make new shell
            toserv = make_sh();
            toproc = toserv + 1;

            proccomm_toserv[npipes] = toserv;
            proccomm_toproc[npipes] = toproc;
            npipes++;
          }
        }
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
