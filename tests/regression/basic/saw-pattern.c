#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <assert.h>

// the main thread creates and joins a thread N times in a loop, which creates a
// pattern similar to that of the teeth of a saw. In fact, each teeth creates K
// concurrent threads.

#ifndef N
#define N 10
#endif

#ifndef K
#define K 2
#endif

void *thread2 (void *arg);

void *thread (void *arg)
{
   long j = (long) arg;
   int i, ret;
   pthread_t th[K];

   printf ("t%ld: running!\n", j);
   for (i = 0; i < K; i++)
   {
      ret = pthread_create (th + i, 0, thread2, (void*) j);
      assert (ret == 0);
   }
   for (i = 0; i < K; i++)
   {
      ret = pthread_join (th[i], 0);
      assert (ret == 0);
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

   for (i = 0; i < N; i++)
   {
      printf ("m: =======\n");
      ret = pthread_create (&th, 0, thread, (void*) i);
      assert (ret == 0);

      ret = pthread_join (th, 0);
      assert (ret == 0);
   }

   pthread_exit (0);
}
