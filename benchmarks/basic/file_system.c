#include "pthread.h"

#define NUMBLOCK 26
#define NUMINODE 32
#define NUM_THREADS 4

pthread_mutex_t locki[NUMINODE];
int inode[NUMINODE];
pthread_mutex_t lockb[NUMBLOCK];
int busy[NUMBLOCK];

pthread_t  tids[NUM_THREADS];

void * thread_routine1()
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
	    busy[b1] = true; 
	    inode[i1] = b1+1; 
	    pthread_mutex_unlock(&lockb[b1]);
	    break;
      }
      pthread_mutex_unlock(&lockb[b1]);
      b1 = (b1+1)%NUMBLOCK; 
    }
  }
  pthread_mutex_unlock(&locki[i1]);
  
  return NULL;
}

void * thread_routine2()
{
  int b2;
  int tid2 = 2;
  int i2 = tid2 % NUMINODE; 
  
  pthread_mutex_lock(&locki[i2]);
  if(inode[i2] == 0) {
    b2 = (i2*2)%NUMBLOCK; 
    while(1) {
      pthread_mutex_lock(&lockb[b2]);
      if(!busy[b2]){
	    busy[b2] = true; 
	    inode[i2] = b2+1; 
	    pthread_mutex_unlock(&lockb[b2]);
	    break;
      }
      pthread_mutex_unlock(&lockb[b2]);
      b2 = (b2+1)%NUMBLOCK; 
    }
  }
  pthread_mutex_unlock(&locki[i2]);
  
  return NULL;
}

void * thread_routine3()
{
  int b3;
  int tid3 = 3;
  int i3 = tid3 % NUMINODE; 
  
  pthread_mutex_lock(&locki[i3]);
  if(inode[i3] == 0) {
    b3 = (i3*2)%NUMBLOCK; 
    while(1) {
      pthread_mutex_lock(&lockb[b3]);
      if(!busy[b3]){
	    busy[b3] = true; 
	    inode[i3] = b3+1; 
	    pthread_mutex_unlock(&lockb[b3]);
	    break;
      }
      pthread_mutex_unlock(&lockb[b3]);
      b3 = (b3+1)%NUMBLOCK; 
    }
  }
  pthread_mutex_unlock(&locki[i3]);
  
  return NULL;
}

void * thread_routine4()
{
  int b4;
  int tid4 = 4;
  int i4 = tid4 % NUMINODE; 
  
  pthread_mutex_lock(&locki[i4]);
  if(inode[i4] == 0) {
    b4 = (i4*2)%NUMBLOCK; 
    while(1) {
      pthread_mutex_lock(&lockb[b4]);
      if(!busy[b4]){
	    busy[b4] = true; 
	    inode[i4] = b4+1; 
	    pthread_mutex_unlock(&lockb[b4]);
	    break;
      }
      pthread_mutex_unlock(&lockb[b4]);
      b4 = (b4+1)%NUMBLOCK; 
    }
  }
  pthread_mutex_unlock(&locki[i4]);

  return NULL;
}

int main ()
{
  int i=0; 

  while (i < NUMINODE)
  {
    pthread_mutex_init(&locki[i], NULL);
    i+=1;  
  }

  i = 0;    
  
  while (i < NUMINODE)
  {
    pthread_mutex_init(&lockb[i], NULL);
    i+=1;  
  }
  
  pthread_create(&tids[0], NULL, thread_routine1, NULL);
  pthread_create(&tids[1], NULL, thread_routine2, NULL);
  pthread_create(&tids[2], NULL, thread_routine3, NULL);
  pthread_create(&tids[3], NULL, thread_routine4, NULL);

  pthread_join(tids[0], NULL);
  pthread_join(tids[1], NULL);
  pthread_join(tids[2], NULL);
  pthread_join(tids[3], NULL);
  
}
