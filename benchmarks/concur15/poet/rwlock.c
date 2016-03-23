/* Testcase from Threader's distribution. For details see:
   http://www.model.in.tum.de/~popeea/research/threader

   This file is adapted from the example introduced in the paper:
   Thread-Modular Verification for Shared-Memory Programs 
   by Cormac Flanagan, Stephen Freund, Shaz Qadeer.
*/

#include "pthread.h"

pthread_mutex_t rwl;

int w=0;
int r=0;
int x; 
int y;


void *writer0() { //writer
  pthread_mutex_lock(rwl);
 L1: 
  if(w ==0){
    if(r ==0)
      w = 1;
    else goto L1;
  } else goto L1;
  pthread_mutex_unlock(rwl);
  x = 3;
  w = 0;
  
  //return NULL;
}

void *writer1() { //writer
  pthread_mutex_lock(rwl);
 L2: 
  if(w ==0){
    if(r ==0)
      w = 1;
    else goto L2;
  } else goto L2;
  pthread_mutex_unlock(rwl);
  x = 3;
  w = 0;
    
  // return NULL;
}

void *reader0() { //reader
  int l0;
  //__VERIFIER_atomic_take_read_lock();
  int t; 
  pthread_mutex_lock(rwl);
 L3:
  if(w==0){
    t = r; 
    r = t+1;
  }else goto L3;
  pthread_mutex_unlock(rwl);
  t = x;
  y = t;
  if (y != t)
    __poet_fail();
  l0 = r-1;
  r = l0;
  
  //  return NULL;
}

void *reader1() { //reader
  int l1;
  //__VERIFIER_atomic_take_read_lock();
  int t; 
  pthread_mutex_lock(rwl);
 L4:
  if(w==0){
    t = r; 
    r = t+1;
  }else goto L4;
  pthread_mutex_unlock(rwl);

  t = x;
  y = t;
  if (y != t)
    __poet_fail();

  l1 = r-1;
  r = l1;
  
  //return NULL;
}

int main() {
    pthread_t t1;
    pthread_t t2;
    pthread_t t3;
    pthread_t t4;

    pthread_mutex_init(rwl, NULL);


    pthread_create(t1, NULL, writer0, NULL);
    pthread_create(t2, NULL, reader0, NULL);
    pthread_create(t3, NULL, writer1, NULL);
    pthread_create(t4, NULL, reader1, NULL);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);
    pthread_join(t4, NULL);
    
    //    return 0;
}
