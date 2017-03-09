#include <pthread.h>
#include <stdio.h>
#include <assert.h>

pthread_mutex_t k;
pthread_mutex_t m;

void *t1(void *arg)
{
 pthread_mutex_lock(&k);
 printf ("t1: lock k\n");
 pthread_mutex_unlock(&k);
 return 0;
}

void *t2(void *arg)
{
 pthread_mutex_lock(&m);
 printf ("t2: lock m\n");
 pthread_mutex_unlock(&m);
 return 0;
}

int main()
{
  pthread_t s;
  pthread_t t;

  pthread_mutex_init(&k, NULL);
  pthread_mutex_init(&m, NULL);

  pthread_create(&s, NULL, t1, NULL);

  pthread_mutex_lock (&k);
  pthread_mutex_unlock (&k);
  pthread_create(&t, NULL, t2, NULL);

  pthread_mutex_lock (&m);
  pthread_mutex_unlock (&m);

  pthread_join(t, NULL);
  pthread_join(s, NULL);

  return 0;
}

