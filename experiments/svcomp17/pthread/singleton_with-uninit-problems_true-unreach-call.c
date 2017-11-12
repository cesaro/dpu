extern void __VERIFIER_error() __attribute__ ((__noreturn__));

#include <stdlib.h>
#include <pthread.h>
#include <string.h>

void __VERIFIER_assert(int expression) { if (!expression) { ERROR: __VERIFIER_error();}; return; }

char *v;
// DPU
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

void *thread1(void * arg)
{
  v = malloc(sizeof(char));
  return 0;
}

void *thread2(void *arg)
{
  // DPU
  pthread_mutex_lock (&m);
  v[0] = 'X';
  pthread_mutex_unlock (&m);
  return 0;
}

void *thread3(void *arg)
{
  pthread_mutex_lock (&m);
  v[0] = 'Y';
  pthread_mutex_unlock (&m);
  return 0;
}

void *thread0(void *arg)
{
  pthread_t t1, t2, t3, t4, t5;

  pthread_create(&t1, 0, thread1, 0);
  pthread_join(t1, 0);
  pthread_create(&t2, 0, thread2, 0);  
  pthread_create(&t3, 0, thread3, 0);
  pthread_create(&t4, 0, thread2, 0);
  pthread_create(&t5, 0, thread2, 0);
  pthread_join(t2, 0);
  pthread_join(t3, 0);
  pthread_join(t4, 0);
  pthread_join(t5, 0);

  return 0;
}

int main(void)
{
  pthread_t t;

  pthread_create(&t, 0, thread0, 0);
  pthread_join(t, 0);

  __VERIFIER_assert(v[0] == 'X' || v[0] == 'Y');

  return 0;
}

