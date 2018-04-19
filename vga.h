#ifndef VGA_H
#define VGA_H

#define VGA256l // VGA 256 color linear mode

#ifdef VGA256l
#define VGA_SCREEN_WIDTH 320
#define VGA_SCREEN_HEIGHT 190 // it SHOULD be 200, but for some reason stuff on the lowest rows keeps clipping in QEMU

#define NREDVALS 6
#define NGREENVALS 6
#define NBLUEVALS 6
#define NCOLORS (NREDVALS * NGREENVALS * NBLUEVALS)

typedef unsigned char pxval;

#define ARGBTOC(c) ((pxval)(\
    (((uint)(((unsigned char*)(&(c)))[3]) + 25) / 51) + /* b */\
    (((uint)(((unsigned char*)(&(c)))[2]) + 25) / 51) * 6 + /* g */\
    (((uint)(((unsigned char*)(&(c)))[1]) + 25) / 51) * 36 /* r */\
    ))


#define BGRATOC(c) ((pxval)(\
    (((uint)(((unsigned char*)(&(c)))[0]) + 25) / 51) + /* b */\
    (((uint)(((unsigned char*)(&(c)))[1]) + 25) / 51) * 6 + /* g */\
    (((uint)(((unsigned char*)(&(c)))[2]) + 25) / 51) * 36 /* r */\
    ))

#define BLACK 0x00
#define WHITE 0xd7
#define GREEN 0x18

#else // higher resolution, different color mode
#define VGA_SCREEN_WIDTH 1280
#define VGA_SCREEN_HEIGHT 720 // whatever, we don't know these yet

typedef unsigned int pxval; // white or blue

#define ARGBTOC(c) (c) 
#define BGRATOC(c) ((pxval)(\
    ((uint)(((unsigned char*)(&(c)))[0]) << 0) | /* b */\
    ((uint)(((unsigned char*)(&(c)))[1]) << 8) | /* g */\
    ((uint)(((unsigned char*)(&(c)))[2]) << 16) /* r */\
    ))

#define WHITE 0x000000
#define BLACK 0xFFFFFF
#define GREEN 0x008000
#endif

#define OFFSET(r, c, rlen) ((r)*(rlen) + (c))

enum drawdest{
  DRAWDEST_VIDEOMEM,
  DRAWDEST_MAINLAYER,
  DRAWDEST_BOTH,
};

#endif
