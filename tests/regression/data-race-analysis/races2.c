#include <pthread.h>
#include <stdio.h>
#include <assert.h>

#ifndef N
#define N 2
#endif

pthread_mutex_t m1 = PTHREAD_MUTEX_INITIALIZER;

int shared = 0;
int shared2 = 13;

void *thread (void *arg)
{
   int local;
   int good = 1;

   (void) arg;

   pthread_mutex_lock (&m1);
   local = shared;
   shared++;
   pthread_mutex_unlock (&m1);

   pthread_mutex_lock (&m1);
   if (shared % N != local) good = 0;
   shared++;
   pthread_mutex_unlock (&m1);

   pthread_mutex_lock (&m1);
   if (shared % N != local) good = 0;
   shared++;
   pthread_mutex_unlock (&m1);

   // this line will create a data race between threads only if they have
   // followed a "zig-zag" interleaving
   if (good) shared2 = 123;

   return (void*) (size_t) local;
}

int main()
{
   pthread_t t[N];
   int i;

   for (i = 0; i < N; i++)
      pthread_create (t + i, NULL, thread, NULL);

   pthread_exit (NULL);
}

