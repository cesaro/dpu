#include <stdio.h>
#include <pthread.h>
#include <assert.h>

#ifndef N
#define N 8
#endif

pthread_t th[N];

void *thread (void *arg)
{
   int ret;
   long i = (long) arg;

   printf ("t%ld: alive!\n", i);

   // if we are the last, we do not create new threads
   if (i >= N) return 0;

   // otherwise we create the thread i + 1
   assert (i < N);
   ret = pthread_create (th + i, 0, thread, (void*) (i + 1));
   assert (ret == 0);
   return 0;
}

int main (int argc, char ** argv)
{
   int ret;
   long i;

   (void) argc;
   (void) argv;

   // create only 1 thread if N >= 1
   if (N >= 1)
   {
      ret = pthread_create (th + 0, 0, thread, (void*) 1);
      assert (ret == 0);
   }

   // we wait for all threads
   for (i = 0; i < N; i++)
   {
      ret = pthread_join (th[i], 0);
      assert (ret == 0);
   }
   return 0;
}
