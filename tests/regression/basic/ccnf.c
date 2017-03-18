#include <stdio.h>
#include <pthread.h>
#include <assert.h>

#ifndef N
#define N 9
#endif

#ifndef JOIN
#define JOIN 1
#endif

pthread_mutex_t m[N];

void *thread (void *arg)
{
   long i = (long) arg;
   int ret;

   // enter cs
   ret = pthread_mutex_lock (m + i/2);
   assert (ret == 0);

   printf ("t%ld: cs %ld\n", i, i/2);

   // exit cs
   ret = pthread_mutex_unlock (m + i/2);
   assert (ret == 0);

   return 0;
}

int main (int argc, char ** argv)
{
   int ret;
   long i;
   pthread_t th[N*2];

   (void) argc;
   (void) argv;

   // if N is even this will initialize one mutex more than necessary ;)
   // as (N/2) != (N-1)/2 in general
   for (i = 0; i < N; i++)
      pthread_mutex_init (m + i, 0);

   for (i = 0; i < N*2; i++)
   {
      ret = pthread_create (th + i, 0, thread, (void*) i);
      assert (ret == 0);
   }

   // we conditionally do join
   if (JOIN)
   {
      for (i = 0; i < N*2; i++)
      {
         ret = pthread_join (th[i], 0);
         assert (ret == 0);
      }
   }

   pthread_exit (0);
}
