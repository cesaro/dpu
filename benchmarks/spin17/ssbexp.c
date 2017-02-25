#include <pthread.h>
#include <stdio.h>
#include <assert.h>

#ifndef PARAM1
#define PARAM1 10 // default value
#endif

#define K PARAM1

pthread_mutex_t ma[K];
pthread_mutex_t mi;
int i = 0;

void *counter()
{
  int x;
  for (x = 1; x < K; x++)
  {
    pthread_mutex_lock(&mi);
    i = x; 
    //printf ("count: i %d\n", i);
    pthread_mutex_unlock(&mi);
  }
  return 0;
}

void *wa(void *aux)
{
  long x = (long) aux;
  pthread_mutex_lock(&ma[x]);
  //printf ("w%ld: locking\n", x);
  pthread_mutex_unlock(&ma[x]);
  return 0;
}

int main()
{
  long x;
  int idx;
  pthread_t idk;
  pthread_t idw[K];

  //printf ("== start ==\n");
  pthread_mutex_init(&mi, NULL);
  for (x = 0; x < K; x++)
  {
    pthread_mutex_init(&ma[x], NULL);
    pthread_create(&idw[x], 0, wa, (void*) x);
  }
  pthread_create(&idk, NULL, counter, NULL);

  pthread_mutex_lock (&mi);
  idx = i;
  //printf ("m: reading %d\n", idx);
  pthread_mutex_unlock (&mi);

  pthread_mutex_lock (&ma[idx]);
  //printf ("m: locking %d\n", idx);
  pthread_mutex_unlock (&ma[idx]);

  for (x = 0; x < K; x++)
    pthread_join(idw[x],NULL);
  pthread_join(idk,NULL);
  //printf ("== end ==\n");
  return 0;
}
