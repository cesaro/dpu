/*  */

#include "pthread.h"

int a=0;
int b=0;
int c=0;
int d=0;
int e=0;
int f=0;
int g=0;
int h=0;

void *t1(){a=1;}
void *t2(){a=2;}
void *t3(){b=1;}
void *t4(){b=2;}
void *t5(){c=1;}
void *t6(){c=2;}
void *t7(){d=1;}
void *t8(){d=2;}
void *t9(){e=1;}
void *t10(){e=2;}
void *t11(){f=1;}
void *t12(){f=2;}
void *t13(){g=1;}
void *t14(){g=2;}
void *t15(){h=1;}
void *t16(){h=2;}

int main()
{
  pthread_t id1,id2,id3,id4,id5,id6,id7,id8,id9,id10,id11,id12,id13,id14,id15,id16;
  pthread_create(&id1, 0, t1, 0);
  pthread_create(&id2, 0, t2, 0);
  pthread_create(&id3, 0, t3, 0);
  pthread_create(&id4, 0, t4, 0);
  pthread_create(&id5, 0, t5, 0);
  pthread_create(&id6, 0, t6, 0);
  pthread_create(&id7, 0, t7, 0);
  pthread_create(&id8, 0, t8, 0);
  pthread_create(&id9, 0, t9, 0);
  pthread_create(&id10, 0, t10, 0);
  pthread_create(&id11, 0, t11, 0);
  pthread_create(&id12, 0, t12, 0);
  pthread_create(&id13, 0, t13, 0);
  pthread_create(&id14, 0, t14, 0);
  pthread_create(&id15, 0, t15, 0);
  pthread_create(&id16, 0, t16, 0);
}
