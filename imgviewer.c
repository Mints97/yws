#include "types.h"
#include "user.h"

// Tell the window manager to draw some image
// This makes the window manager act as a mix
// between Xorg and an actual window manager
int
main(int argc, char *argv[])
{
  if(argc > 1) {
    printf(4, "drawbmp %s", argv[1]);
    while(1);
  }
  exit();
}
