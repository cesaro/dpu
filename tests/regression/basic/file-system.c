
#include <pthread.h>
#include <stdio.h>
#include <assert.h>

#ifndef N
#define N 13
#endif

#define NUMBLOCK 26
#define NUMINODE 32
#define NUM_THREADS N

/*
 * This bechmark has non-terminating executions if the number of threads is
 * greater than the number of blocks. Each thread will pick an inode and will
 * try to search for a block for it using a while(1) loop that does not detect
 * the condition of not having a block available at all.
 */

pthread_mutex_t locki[NUMINODE];
int inode[NUMINODE];

pthread_mutex_t lockb[NUMBLOCK];
int busy[NUMBLOCK];

void *thread_routine (void *arg)
{
  int b;
  long tid = (long) arg;
  int i = tid % NUMINODE; 

  printf ("t%ld: starting, inode %d\n", tid, i);
  
  pthread_mutex_lock (&locki[i]);
  if (inode[i] == 0)
  {
    b = (i*2) % NUMBLOCK; 
    while (1) {
      printf ("t%ld: checking block %d\n", tid, b);
      pthread_mutex_lock (&lockb[b]);
      if (!busy[b]) {
        busy[b] = 1; // true
        inode[i] = b+1; 
        pthread_mutex_unlock (&lockb[b]);
        break;
      }
      pthread_mutex_unlock (&lockb[b]);
      b = (b+1) % NUMBLOCK; 
    }
  }
  pthread_mutex_unlock (&locki[i]);
  
  return 0;
}

int main ()
{
  pthread_t tids[NUM_THREADS];
  int i, ret;

  // one mutex per inode
  for (i = 0; i < NUMINODE; ++i)
  {
    ret = pthread_mutex_init(&locki[i], 0);
    assert (ret == 0);
  }

  // one mutex per block
  for (i = 0; i < NUMBLOCK; ++i)
  {
    ret = pthread_mutex_init(&lockb[i], 0);
    assert (ret == 0);
  }
  
  // NUM_THREADS threads
  for (i = 0; i < NUM_THREADS; ++i)
  {
    ret =pthread_create(&tids[i], 0, thread_routine, (void*) (long) i);
    assert (ret == 0);
  }

  for (i = 0; i < NUM_THREADS; ++i)
  {
    ret = pthread_join (tids[i], 0);
    assert (ret == 0);
  }

  return 0;
}
