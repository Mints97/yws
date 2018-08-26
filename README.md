# YWS

The Y Window System is a simple tile-based window system for the xv6 OS.

To launch, run "make qemu".

Ctrl ' splits a tile horizontally, Ctrl , splits it vertically, Ctrl - closes the current tile, Ctrl = opens a new shell.
Tiles can be selected by mouse, the selected tile is highlighted in green.
Tiles can be resized by dragging the boundaries with a mouse.

A program launched in a shell can use file descriptors 3 and 4 to communicate with the window server and send it commands.
An example is the imgviewer program, which can display bmp files.
Usage: "imgviewer path-to-image". Examples: "imgviewer diddy.bmp", "imgviewer y_u_no.bmp".

![example usage](https://raw.githubusercontent.com/Mints97/yws/spring2018/display_example.png)

A full description is available here: https://github.com/Mints97/yws/blob/spring2018/CS%203210%20-%20Final%20Project%20Writeup.pdf

# xv6
xv6 is a re-implementation of Dennis Ritchie's and Ken Thompson's Unix
Version 6 (v6).  xv6 loosely follows the structure and style of v6,
but is implemented for a modern x86-based multiprocessor using ANSI C.

ACKNOWLEDGMENTS

xv6 is inspired by John Lions's Commentary on UNIX 6th Edition (Peer
to Peer Communications; ISBN: 1-57398-013-7; 1st edition (June 14,
2000)). See also http://pdos.csail.mit.edu/6.828/2016/xv6.html, which
provides pointers to on-line resources for v6.

xv6 borrows code from the following sources:
    JOS (asm.h, elf.h, mmu.h, bootasm.S, ide.c, console.c, and others)
    Plan 9 (entryother.S, mp.h, mp.c, lapic.c)
    FreeBSD (ioapic.c)
    NetBSD (console.c)

The following people have made contributions: Russ Cox (context switching,
locking), Cliff Frey (MP), Xiao Yu (MP), Nickolai Zeldovich, and Austin
Clements.

We are also grateful for the bug reports and patches contributed by Silas
Boyd-Wickizer, Mike CAT, Nelson Elhage, Nathaniel Filardo, Peter Froehlich,
Yakir Goaran, Shivam Handa, Bryan Henry, Jim Huang, Anders Kaseorg, kehao95,
Eddie Kohler, Imbar Marinescu, Yandong Mao, Hitoshi Mitake, Carmi Merimovich,
Joel Nider, Greg Price, Ayan Shafqat, Eldar Sehayek, Yongming Shen, Cam Tenny,
Rafael Ubal, Warren Toomey, Stephen Tu, Pablo Ventura, Xi Wang, Keiichi
Watanabe, Nicolas Wolovick, Jindong Zhang, and Zou Chang Wei.

The code in the files that constitute xv6 is
Copyright 2006-2016 Frans Kaashoek, Robert Morris, and Russ Cox.

ERROR REPORTS

If you spot errors or have suggestions for improvement, please send
email to Frans Kaashoek and Robert Morris (kaashoek,rtm@csail.mit.edu).

BUILDING AND RUNNING XV6

To build xv6 on an x86 ELF machine (like Linux or FreeBSD), run "make".
On non-x86 or non-ELF machines (like OS X, even on x86), you will
need to install a cross-compiler gcc suite capable of producing x86 ELF
binaries.  See http://pdos.csail.mit.edu/6.828/2016/tools.html.
Then run "make TOOLPREFIX=i386-jos-elf-".

To run xv6, install the QEMU PC simulators.  To run in QEMU, run "make qemu".

To create a typeset version of the code, run "make xv6.pdf".  This
requires the "mpage" utility.  See http://www.mesa.nl/pub/mpage/.
