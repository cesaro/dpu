#include <pthread.h>
#include <stdio.h>
#include <assert.h>

void *ta (void *arg);
void *tb (void *arg);
void *tc (void *arg);
void *td (void *arg);

pthread_mutex_t m1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t m2 = PTHREAD_MUTEX_INITIALIZER;

void *ta (void *arg)
{
   pthread_t c;

   (void) arg;
   printf ("ta: running\n");

   // triggers a race between thread a and b
   pthread_mutex_lock (&m1);
   pthread_mutex_unlock (&m1);

   pthread_create (&c, NULL, tc, NULL);
   return 0;
}

void *tb (void *arg)
{
   pthread_t d;

   (void) arg;
   printf ("tb: running\n");

   pthread_create (&d, NULL, td, NULL);

   // triggers a race between thread a and b
   pthread_mutex_lock (&m1);
   pthread_mutex_unlock (&m1);

   // triggers a race between threads b and d
   pthread_mutex_lock (&m2);
   pthread_mutex_unlock (&m2);
   return 0;
}

void *tc (void *arg)
{
   (void) arg;
   printf ("tc: running\n");
   return 0;
}

void *td (void *arg)
{
   (void) arg;
   printf ("td: running\n");
   // triggers a race between threads b and d
   pthread_mutex_lock (&m2);
   pthread_mutex_unlock (&m2);
   return 0;
}

int main()
{
   pthread_t a, b;

   pthread_create (&a, NULL, ta, NULL);
   pthread_create (&b, NULL, tb, NULL);
   pthread_exit (NULL);
}

