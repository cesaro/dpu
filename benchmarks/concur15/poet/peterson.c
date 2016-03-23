/* Adapted from: https://svn.sosy-lab.org/software/sv-benchmarks/trunk/c/pthread-atomic/peterson_true-unreach-call.c */

#include "pthread.h"

int flag1 = 0, flag2 = 0; // boolean flags
int turn; // integer variable to hold the ID of the thread whose turn is it
int x; // boolean variable to test mutual exclusion

void *thr1() {
  flag1 = 1;
  turn = 1;
  int l1=flag2;
  while (l1==1 && turn==1) {l1=flag2;}
  // begin: critical section
  x = 0;
  if(x>0){
      __poet_fail(); //   assert(x<=0);
  }
  // end: critical section
  flag1 = 0;
}

void *thr2() {
  flag2 = 1;
  turn = 0;
  int l2=flag1;
  while (l2==1 && turn==0) {l2=flag1;}
  // begin: critical section
  x = 1;
  if(x<1){
      __poet_fail(); //   assert(x>=1);
  }
  // end: critical section
  flag2 = 0;
}
  
int main() {
  pthread_t t1, t2;
  pthread_create(t1, NULL, thr1, NULL);
  pthread_create(t2, NULL, thr2, NULL);
  pthread_join(t1, NULL);
  pthread_join(t2, NULL);
}
