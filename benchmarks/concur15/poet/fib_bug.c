/* Adapted from: https://svn.sosy-lab.org/software/sv-benchmarks/trunk/c/pthread/fib_bench_false-unreach-call.c */

#include "pthread.h"

int i=1, j=1;

#define NUM 5

void *t1()
{
  int k = 0;
  int s, t;
  for (k = 0; k < NUM; k=k+1){
    s = j;
    t = i+s;
    i=t;
  }
}

void *t2()
{
  int k = 0;

  int t;
  int s;
  for (k = 0; k < NUM; k=k+1){
    t = j;
    s = t + i;
    j = s;
  }


}

int
main(int argc, char **argv)
{

  pthread_t id1, id2;

  pthread_create(id1, NULL, t1, NULL);
  pthread_create(id2, NULL, t2, NULL);
  pthread_join(id1,NULL);
  pthread_join(id2,NULL);

  int l=i;
  if (l >= 144 || j >= 144) { 
      __poet_fail();
  } 

}
