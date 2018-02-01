extern void __VERIFIER_assume(int);
extern void __VERIFIER_error() __attribute__ ((__noreturn__));

#include <stdlib.h>
#include <pthread.h>
#include <string.h>

void __VERIFIER_assert(int expression) { if (!expression) { ERROR: __VERIFIER_error();}; return; }

// DPU: sigma reduced from 16 to 5
const int SIGMA = 5;

int *array;
int array_index=-1;

// DPU
pthread_mutex_t mutexes[SIGMA];
pthread_mutex_t mutex_index = PTHREAD_MUTEX_INITIALIZER; 

void *thread(void * arg)
{
   int i;
   // DPU: data race removed by 2 mutexes
   pthread_mutex_lock (&mutex_index);
   i = array_index;
   pthread_mutex_unlock (&mutex_index);

   pthread_mutex_lock (mutexes + i);
   array[i] = 1;
   pthread_mutex_unlock (mutexes + i);
   return 0;
}

int main()
{
   int tid, sum;
   pthread_t *t;

   t = (pthread_t *)malloc(sizeof(pthread_t) * SIGMA);
   array = (int *)malloc(sizeof(int) * SIGMA);

   __VERIFIER_assume(t != 0);
   __VERIFIER_assume(array != 0);

   // DPU initialize the mutexes
   for (tid = 0; tid < SIGMA; tid++) pthread_mutex_init (mutexes + tid, 0);

   for (tid=0; tid<SIGMA; tid++) {
      pthread_mutex_lock (&mutex_index);
      array_index++;
      pthread_mutex_unlock (&mutex_index);
      pthread_create(&t[tid], 0, thread, 0);
   }

   for (tid=0; tid<SIGMA; tid++) {
      pthread_join(t[tid], 0);
   }

   for (tid=sum=0; tid<SIGMA; tid++) {
      sum += array[tid];
   }

   __VERIFIER_assert(sum == SIGMA);  // <-- wrong, different threads might use the same array offset when writing

   return 0;
}

