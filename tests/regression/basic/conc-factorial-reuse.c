#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <assert.h>

#ifndef N
#define N 5
#endif

#ifndef K
#define K 4
#endif

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

void *thread2 (void *arg);

void *thread (void *arg)
{
   long j = (long) arg;
   int i, ret;
   pthread_t th[K];

   pthread_mutex_lock(&m);
   pthread_mutex_unlock(&m);

   printf ("t%ld: running!\n", j);
   for (i = 0; i < K; i++)
   {
      ret = pthread_create (th + i, 0, thread2, (void*) j);
      assert (ret == 0);

      // we only join for the first two, so the third will consume a new slot
      if (i < 2)
      {
         ret = pthread_join (th[i], 0);
         assert (ret == 0);
      }
   }
   return 0;
}

void *thread2 (void *arg)
{
   long i = (long) arg;

   printf ("t%ld': running!\n", i);
   return 0;
}

int main (int argc, char ** argv)
{
   int ret;
   long i;
   pthread_t th;

   (void) argc;
   (void) argv;

   ret = pthread_mutex_init (&m, 0);
   assert (ret == 0);

   for (i = 0; i < N; i++)
   {
      ret = pthread_create (&th, 0, thread, (void*) i);
      assert (ret == 0);
   }
   pthread_exit (0);
}
