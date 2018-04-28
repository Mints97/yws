#include "types.h"
#include "defs.h" 
#include "spinlock.h"
#include "keventq.h"

// Mouse and keyboard events for the entire visual interface

static struct eventq
{
  int head;
  int tail;
  int size;

  int events[EVENTQSIZE];
  struct spinlock qlock;
} queue;

// ======= WARNING: DO NOT put debug print statments inside critical sections in
// these functions! This deadlocks this event queue AND the console! ====

void
initeventq(void)
{
  queue.head = queue.tail = queue.size = 0;

  initlock(&(queue.qlock), "event_queue");
}

// Enqueue an event
// This will simply fail when event queue will get full
static int 
enqevent(int event)
{
  acquire(&(queue.qlock));
  if(queue.size >= EVENTQSIZE){
    release(&(queue.qlock));
    return -1;
  }
  queue.tail = (queue.tail + 1) % EVENTQSIZE;
  queue.events[queue.tail] = event;
  queue.size++;
  release(&(queue.qlock));
  return 0;
}

int
enq_cursormove(ushort x, ushort y)
{ // format: highest bit set to 1
  return enqevent((1 << 31) | (((uint)x) << 16) | y);
}

int
enq_cursorclick(int lbtn, int rbtn, int mbtn)
{
  return enqevent((1 << 30) | ((!!lbtn) << 2) | ((!!rbtn) << 1) | (!!mbtn));
}

int
enq_kbevent(uchar c, int released, int shiftheld, int ctlheld)
{
  return enqevent((1 << 29) | ((!!released) << 10) | ((!!shiftheld) << 9) | ((!!ctlheld) << 8) | (((ushort)c) & 0xFF));
}

// Dequeue an event
// Will fail when queue is empty
int
deqevent(void)
{
  // we assume one consumer here.
  acquire(&(queue.qlock));
  if(queue.size <= 0){
    release(&(queue.qlock));
    return -1;
  }
  int result = queue.events[queue.head];
  queue.head = (queue.head + 1) % EVENTQSIZE;
  queue.size--;
  release(&(queue.qlock));

  return result;
}
