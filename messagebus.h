#ifndef MESSAGEBUS_H
#define MESSAGEBUS_H

typedef enum {PID_CREATE, PID_DIE} pidenum;

typedef struct _pidevent{
    pidenum event;
    int pid;
} pidevent;


void externputevent(pidenum type, int);
int messageread(struct inode *, char *, int);
int messagewrite(struct inode *, char *, int);
void mbusinit(void);

#endif  // MESSAGEBUS_H