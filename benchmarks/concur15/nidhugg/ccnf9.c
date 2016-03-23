/*  */

#include "pthread.h"

int a=0;
int b=0;
int c=0;
int d=0;

void *t1(){a=1;}
void *t2(){a=2;}
void *t3(){b=1;}
void *t4(){b=2;}
void *t5(){c=1;}
void *t6(){c=2;}
void *t7(){d=1;}
void *t8(){d=2;}

int main()
{
  pthread_t id1,id2,id3,id4,id5,id6,id7,id8;
  pthread_create(&id1, 0, t1, 0);
  pthread_create(&id2, 0, t2, 0);
  pthread_create(&id3, 0, t3, 0);
  pthread_create(&id4, 0, t4, 0);
  pthread_create(&id5, 0, t5, 0);
  pthread_create(&id6, 0, t6, 0);
  pthread_create(&id7, 0, t7, 0);
  pthread_create(&id8, 0, t8, 0);
}
