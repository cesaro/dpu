
#include <stdio.h>
#include <pthread.h>
#include <assert.h>

#ifndef N
#define N 5
#endif

// N  max confs (according to nidhugg)
// 1  2
// 2  6
// 3  20
// 4  70
// 5  252
// 6  924
// 7  3432
// 8  12870

pthread_mutex_t m;
int count = 0;

void *thread (void *arg)
{
   int ret, i;
   long j = (long) arg;
   int done = 0;

   for (i = 0; i < N; i++)
   {
      // enter cs
      ret = pthread_mutex_lock (&m);
      assert (ret == 0);

      count++;
      printf ("t%ld: in cs, i %d, count %d\n", j, i, count);
      if (count == 2*N)
      {
         done = 1;
      }

      // exit cs
      ret = pthread_mutex_unlock (&m);
      assert (ret == 0);
   }

   if (done)
   {
      ret = pthread_mutex_destroy (&m);
      assert (ret == 0);
   }
   return 0;
}

int main (int argc, char ** argv)
{
   int ret;
   pthread_t th;

   (void) argc;
   (void) argv;

   ret = pthread_mutex_init (&m, 0);
   assert (ret == 0);

   ret = pthread_create (&th, 0, thread, (void*) 1);
   assert (ret == 0);

   // we do the second thread
   thread ((void*) 0);

   // we do not wait for our children
   pthread_exit (0);
   return 0;
}
