#include <stdio.h>
#include <pthread.h>
#include <assert.h>

#ifndef N
#define N 1
#endif

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
unsigned count = 0;

void *thread (void *arg)
{
   long i = (long) arg;
   int ret;

   printf ("t%ld: running!\n", i);

   // enter cs
   ret = pthread_mutex_lock (&m);
   assert (ret == 0);

   // increment the count
   count++;

   // exit cs
   ret = pthread_mutex_unlock (&m);
   assert (ret == 0);

   return 0;
}

int main (int argc, char ** argv)
{
   int ret;
   long i;
   pthread_t th[N];

   (void) argc;
   (void) argv;

   ret = pthread_mutex_init (&m, 0);
   assert (ret == 0);

   for (i = 0; i < N; i++)
   {
      ret = pthread_create (th + i, 0, thread, (void*) i);
      assert (ret == 0);
   }

   for (i = 0; i < N; i++)
   {
      ret = pthread_join (th[i], 0);
      assert (ret == 0);
   }

   printf ("m: done, count %d N %d\n", count, N);
   assert (count == N);
   return 0;
}

