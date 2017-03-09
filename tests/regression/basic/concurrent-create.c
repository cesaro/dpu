#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <assert.h>

#ifndef N
#define N 8
#endif

pthread_mutex_t m;

void *thread2 (void *arg)
{
   long i = (long) arg;

   printf ("u: running, i %ld!\n", i);
   return 0;
}

void *thread (void *arg)
{
   long i = (long) arg;
   pthread_t th;
   int ret;

   printf ("t%ld: running!\n", i);

   // create a new thread
   ret = pthread_create (&th, 0, thread2, arg);
   assert (ret == 0);

   // only even threads wait for their child
   if (i % 2 == 0)
   {
      ret = pthread_join (th, 0);
      assert (ret == 0);
   }

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

   // we do not wait for our children
   pthread_exit (0);
}

