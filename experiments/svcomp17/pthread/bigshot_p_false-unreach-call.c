extern void __VERIFIER_error() __attribute__ ((__noreturn__));

#include <stdlib.h>
#include <pthread.h>
#include <string.h>

void __VERIFIER_assert(int expression) { if (!expression) { ERROR: __VERIFIER_error();}; return; }

// DPU:
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

char *v;

void *thread1(void * arg)
{
  // DPU
  pthread_mutex_lock (&m);
  v = malloc(sizeof(char) * 8);
  pthread_mutex_unlock (&m);
  return 0;
}

void *thread2(void *arg)
{
  char *vv;

  // DPU
  pthread_mutex_lock (&m);
  vv = v;
  pthread_mutex_unlock (&m);

  if (vv)
  {
     pthread_mutex_lock (&m);
     vv = v;
     pthread_mutex_unlock (&m);
     strcpy(vv, "Bigshot");
  }
  return 0;
}


int main()
{
  pthread_t t1, t2;

  pthread_create(&t1, 0, thread1, 0);
  pthread_create(&t2, 0, thread2, 0);
  pthread_join(t1, 0);
  pthread_join(t2, 0);

  __VERIFIER_assert(!v || v[0] == 'B');

  return 0;
}

