#include "types.h"
#include "defs.h"
#include "param.h"
#include "traps.h"
#include "spinlock.h"
#include "fs.h"
#include "file.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "messagebus.h"

#define INPUT_BUF 128

struct spinlock mbuslock;

struct {
  pidevent buf[INPUT_BUF];
  uint r;  // Read index
  uint w;  // Write index
} input;

void putevent(pidenum type, int pid) {
    if (input.r == (input.w + 1) % INPUT_BUF) // drop an event
        ++input.r;
    input.buf[input.w++ % INPUT_BUF] = (pidevent) {type, pid};
    input.r %= INPUT_BUF;
    input.w %= INPUT_BUF;
}

void externputevent(pidenum type, int pid) {
    acquire(&mbuslock);
    putevent(type, pid);
    release(&mbuslock);
}

int
messageread(struct inode *ip, char *dst, int n)
{
  if (n <= 0) 
    return -1;

  if (n % sizeof(pidevent) != 0)
    return -1;

  uint target;
  pidevent event;
  iunlock(ip);
  target = n;
  acquire(&mbuslock);
  while (n > 0) {
      while (input.r == input.w) {
          if (proc->killed) {
              release(&mbuslock);
              ilock(ip);
              return -1;
          }
          sleep(&input.r, &mbuslock);
      }
      event = input.buf[input.r++ % INPUT_BUF];
      dst = memmove(dst, &event, sizeof(event));
      n -= sizeof(event);
  }
  release(&mbuslock);
  ilock(ip);

  return target - n;
}

int
messagewrite(struct inode *ip, char *buf, int n)
{
  int i;
  if (n <= 0)
    return -1;

  if (n % sizeof(pidevent) != 0)
    return -1;

  iunlock(ip);
  acquire(&mbuslock);
  for (i = 0; i < n; i += sizeof(pidevent)) {
    putevent(((pidevent *) buf)->event, ((pidevent *) buf)->pid);
    buf += sizeof(pidevent);
  }
  release(&mbuslock);
  ilock(ip);

  return n;
}

void
mbusinit(void)
{
  initlock(&mbuslock, "mbus");
  input.r = 0;
  input.w = 0;
  devsw[MESSAGEBUS].write = messagewrite;
  devsw[MESSAGEBUS].read = messageread;
}