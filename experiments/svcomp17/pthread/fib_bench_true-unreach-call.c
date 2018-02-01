extern void __VERIFIER_error() __attribute__ ((__noreturn__));

#include <pthread.h>

int i=1, j=1;

#define NUM 5

// DPU
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

void *
t1(void* arg)
{
  int k = 0;

  for (k = 0; k < NUM; k++)
  {
    // DPU
    pthread_mutex_lock(&m);
    i+=j;
    pthread_mutex_unlock(&m);
  }

  pthread_exit(NULL);
}

void *
t2(void* arg)
{
  int k = 0;

  for (k = 0; k < NUM; k++)
  {
    // DPU
    pthread_mutex_lock(&m);
    j+=i;
    pthread_mutex_unlock(&m);
  }
  pthread_exit(NULL);
}

int
main(int argc, char **argv)
{
  pthread_t id1, id2;

  pthread_create(&id1, NULL, t1, NULL);
  pthread_create(&id2, NULL, t2, NULL);

  // DPU
  pthread_mutex_lock(&m);
  if (i > 144 || j > 144) {
    ERROR: __VERIFIER_error();
  }
  pthread_mutex_unlock(&m);

  // DPU
  pthread_exit (0);
  return 0;
}
