#include "types.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  if(argc > 1) {
    printf(4, "drawbmp %s", argv[1]);
    while(1);
  }
  exit();
}
