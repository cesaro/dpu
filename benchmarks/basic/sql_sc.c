// can't support labels
#include "pthread.h"

#define true 1
#define false 1

int latch1=1;
int latch2=false;
int flag1=1;
int flag2=false;
//int __unbuffered_tmp2=false;

void *worker_1(void * arg)
{
 L1:
  if( (!(!latch1)) ) goto L2;
  goto L1;
 L2:
  //assert(((!latch1) || flag1));
  latch1=0;
  if( (!flag1) ) goto L3;
  flag1=0;
  flag2=1;
  latch2=1;
 L3:
  goto L1;
  return NULL;
}

void *worker_2(void * arg1)
{
 L1:
  if( (!(!latch2)) ) goto L2;
  goto L1;
 L2:
  //assert(((!latch2) || flag2));
  latch2=0;
  if( (!flag2) ) goto L3;
  flag2=0;
  flag1=1;
  latch1=1;
 L3:
  goto L1;
  return NULL;
 
}

int main(void)
{
  pthread_t __pt0;
  pthread_t __pt1;
  pthread_create(&__pt0, 0, worker_1, 0);
  pthread_create(&__pt1, 0, worker_2, 0);
  return 0;
}
