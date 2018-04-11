#define VGA_SCREEN_WIDTH 320
#define VGA_SCREEN_HEIGHT 200

#define NREDVALS 6
#define NGREENVALS 6
#define NBLUEVALS 6
#define NCOLORS (NREDVALS * NGREENVALS * NBLUEVALS)

#define ARGBTOC(c) ((unsigned char)(\
    (((uint)(((unsigned char*)(&(c)))[3]) + 25) / 51) + /* b */\
    (((uint)(((unsigned char*)(&(c)))[2]) + 25) / 51) * 6 + /* g */\
    (((uint)(((unsigned char*)(&(c)))[1]) + 25) / 51) * 36 /* r */\
    ))


#define BGRATOC(c) ((unsigned char)(\
    (((uint)(((unsigned char*)(&(c)))[0]) + 25) / 51) + /* b */\
    (((uint)(((unsigned char*)(&(c)))[1]) + 25) / 51) * 6 + /* g */\
    (((uint)(((unsigned char*)(&(c)))[2]) + 25) / 51) * 36 /* r */\
    ))

#define OFFSET(r, c, rlen) ((r)*(rlen) + (c))
