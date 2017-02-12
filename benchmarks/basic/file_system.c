
#include <pthread.h>
#include <assert.h>

#define NUMBLOCK 26
#define NUMINODE 32
#define NUM_THREADS 6

pthread_mutex_t locki[NUMINODE];
int inode[NUMINODE];
pthread_mutex_t lockb[NUMBLOCK];
int busy[NUMBLOCK];

void * thread_routine()
{
  int b1;
  int tid1 = 1;
  int i1 = tid1 % NUMINODE; 
  
  pthread_mutex_lock(&locki[i1]);
  if(inode[i1] == 0) {
    b1 = (i1*2)%NUMBLOCK; 
    while(1) {
      pthread_mutex_lock(&lockb[b1]);
      if(!busy[b1]){
        busy[b1] = 1; // true
        inode[i1] = b1+1; 
        pthread_mutex_unlock(&lockb[b1]);
        break;
      }
      pthread_mutex_unlock(&lockb[b1]);
      b1 = (b1+1)%NUMBLOCK; 
    }
  }
  pthread_mutex_unlock(&locki[i1]);
  
  return 0;
}

int main ()
{
  pthread_t tids[NUM_THREADS];
  int i=0; 

  for (i = 0; i < NUMINODE; ++i)
  {
    pthread_mutex_init(&locki[i], 0);
  }

  for (i = 0; i < NUMINODE; ++i)
  {
    pthread_mutex_init(&lockb[i], 0);
  }
  
  for (i = 0; i < NUM_THREADS; ++i)
  {
    pthread_create(&tids[i], 0, thread_routine, 0);
  }

  for (i = 0; i < NUM_THREADS; ++i)
  {
    pthread_join (tids[i], 0);
  }

  return 0;
}
