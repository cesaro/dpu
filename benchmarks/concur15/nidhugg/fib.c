/* Adapted from: https://svn.sosy-lab.org/software/sv-benchmarks/trunk/c/pthread/fib_bench_true-unreach-call.c */

#include <pthread.h>
#include <assert.h>

int i=1; 
int j=1;

#define NUM 5

void *t1()
{
  int k; 
  k = 0;
  int t;
  int s;

  for (k = 0; k < NUM; k=k+1){
    t = j;
    s = i + t; 
    i= s;
  }
}

void *t2()
{
  int k;
  k = 0;
  int t;
  int s;
  for (k = 0; k < NUM; k=k+1){
    t = j;
    s = t + i;
    j = s;
  }
}

int
main()
{
  pthread_t id1;
  pthread_t id2;

  pthread_create(&id1, NULL, t1, NULL);
  pthread_create(&id2, NULL, t2, NULL);

  pthread_join(id1,NULL);
  pthread_join(id2,NULL);

  int l=i;
  if (l > 144 || j > 144) { 
      assert(0);
  } 
}
