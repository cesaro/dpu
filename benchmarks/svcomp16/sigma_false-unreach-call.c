extern void __VERIFIER_assume(int);
extern void __VERIFIER_error();

#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdio.h>

void __VERIFIER_assert(int expression) { if (!expression) { ERROR: __VERIFIER_error();}; return; }

const int SIGMA = 14;

int *array;
int array_index=-1;


void *thread(void * arg)
{
   printf ("t: ai %d\n", array_index);
   array[array_index] = 1;
   return 0;
}


int main()
{
   int tid, sum;
   pthread_t *t;
   t = (pthread_t *)malloc(sizeof(pthread_t) * SIGMA);
   array = (int *)malloc(sizeof(int) * SIGMA);

   __VERIFIER_assume((int) t);
   __VERIFIER_assume((int) array);

   for (tid=0; tid<SIGMA; tid++) {
      printf ("m: tid %d\n", tid);
      array_index++;
      pthread_create(&t[tid], 0, thread, 0);
   }

   for (tid=0; tid<SIGMA; tid++) {
      pthread_join(t[tid], 0);
   }

   for (tid=sum=0; tid<SIGMA; tid++) {
      sum += array[tid];
   }
   printf ("m: sum %d\n", sum);

   __VERIFIER_assert(sum == SIGMA);  // <-- wrong, different threads might use the same array offset when writing

   return 0;
}

