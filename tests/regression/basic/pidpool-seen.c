#include <pthread.h>
#include <stdio.h>
#include <assert.h>

// 3 executions, revealed a bug in the Pidpool::create() method

void *ta (void *arg);
void *tb (void *arg);

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

void *ta (void *arg)
{
   (void) arg;
   printf ("ta: running\n");

   pthread_mutex_lock (&m);
   pthread_mutex_unlock (&m);

   return 0;
}

void *tb (void *arg)
{
   (void) arg;
   printf ("tb: running\n");
   return 0;
}

int main()
{
   pthread_t a, b;

   pthread_create (&b, 0, tb, 0);
   pthread_join (b, 0);

   pthread_create (&a, 0, ta, 0);

   pthread_mutex_lock (&m);
   pthread_mutex_unlock (&m);
   pthread_mutex_lock (&m);
   pthread_mutex_unlock (&m);

   pthread_exit (0);
}


