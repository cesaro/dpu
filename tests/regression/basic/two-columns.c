
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

void *thread (void *arg)
{
   int ret, i;
   long j = (long) arg;

   for (i = 0; i < N; i++)
   {
      // enter cs
      ret = pthread_mutex_lock (&m);
      assert (ret == 0);

      printf ("t%ld: in cs, i %d\n", j, i);

      // exit cs
      ret = pthread_mutex_unlock (&m);
      assert (ret == 0);
   }
   return 0;
}

int main (int argc, char ** argv)
{
   int ret;
   long i;
   pthread_t th[2];

   (void) argc;
   (void) argv;

   ret = pthread_mutex_init (&m, 0);
   assert (ret == 0);

   for (i = 0; i < 1; i++)
   {
      ret = pthread_create (th + i, 0, thread, (void*) i);
      assert (ret == 0);
   }

   // we do the second thread
   thread ((void*) 1);

   // we do not wait for our children
   pthread_exit (0);
}


