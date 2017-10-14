#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifndef N
#define N 3
#endif

void *thread (void *arg)
{
   long i = (long) arg;

   printf ("t%ld: running!\n", i);

   // should report a defect
   if (i == 0) abort ();

   return 0;
}

int main (int argc, char ** argv)
{
   int ret;
   long i;
   void *retval;
   pthread_t th[N];

   (void) argc;
   (void) argv;

   for (i = 0; i < N; i++) pthread_create (th + i, 0, thread, (void*) i);
   for (i = 0; i < N; i++) pthread_join (th[i], 0);

   return 0;
}

