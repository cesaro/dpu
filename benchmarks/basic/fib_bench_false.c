#include "pthread.h"

int i=1;
int j=1;

#define NUM 5

void *t1()
{
  int k1 = 0;

  while (k1 < NUM){
      i=i+j;
      k1=k1+1;
  }

  return NULL;
}

void *t2()
{
  int k2 = 0;

  while(k2 < NUM)
  {
      j=j+i;
      k2=k2+1;
  }

  return NULL;
}

int
main(int argc, char **argv)
{
    pthread_t id1;
    pthread_t id2;

  pthread_create(&id1, NULL, t1, NULL);
  pthread_create(&id2, NULL, t2, NULL);

  if (i >= 144 || j >= 144) {
      return -1;
    //   goto ERROR;
  }

  return 0;
}
