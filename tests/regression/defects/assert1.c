#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <assert.h>

#ifndef N
#define N 4
#endif

int rets[N];

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

void *thread (void *arg)
{
   long i = (long) arg;

   pthread_mutex_lock (&m);
   pthread_mutex_unlock (&m);

   printf ("t%ld: running!\n", i);
   return rets + i;
}

int main (int argc, char ** argv)
{
   int ret;
   long i;
   void *retval;
   pthread_t th[N];

   (void) argc;
   (void) argv;

   for (i = 0; i < N; i++)
   {
      ret = pthread_create (th + i, 0, thread, (void*) i);
      assert (ret == 0);
   }

   for (i = 0; i < N; i++)
   {
      ret = pthread_join (th[i], &retval);
      assert (ret == 0);
      printf ("m: retval %p\n", retval);
      printf ("m: rets + i %p i %ld\n", rets + i, i);

      // should fail
      assert (retval == rets + 0);
   }

   return 0;
}

