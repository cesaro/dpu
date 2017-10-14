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

   printf ("t: running!\n");

   // should report a defect
   abort ();

   return 0;
}

int main (int argc, char ** argv)
{
   int ret;
   long i;
   void *retval;
   pthread_t th;

   (void) argc;
   (void) argv;

   pthread_create (&th, 0, thread, 0);
   pthread_join (th, &retval);

   // should fail and trigger another call to abort
   assert (retval == (void*) 0x123);

   return 0;
}

