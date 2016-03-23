/* Testcase from Threader's distribution. For details see:
   http://www.model.in.tum.de/~popeea/research/threader
*/
#include "pthread.h"

int flag1 = 0;
int flag2 = 0; // boolean flags
int turn = 0; // integer variable to hold the ID of the thread whose turn is it
int x = 0; // boolean variable to test mutual exclusion

void *thr1() {
  flag1 = 1;
  while (flag2 >= 1) {
    if (turn != 0) {
      flag1 = 0;
      //while (turn != 0) {}; // poet modified
		thr1_spinloop:
		if (turn != 0) goto thr1_spinloop;
      flag1 = 1;
    }
  }
  // begin: critical section
  x = 0;
  //assert(x<=0);
  // end: critical section
  turn = 1;
  flag1 = 0;
  
  // return NULL; poet
}

void *thr2() {
  flag2 = 1;
  while (flag1 >= 1) {
    if (turn != 1) {
      flag2 = 0;
      // while (turn != 1) {}; // poet modified
		thr2_spinloop:
		if (turn != 1) goto thr2_spinloop;
      flag2 = 1;
    }
  }
  // begin: critical section
  x = 1;
  // assert(x>=1);
  // end: critical section
  turn = 0;
  flag2 = 0;
  
  // return NULL; // poet
}

int main() {
  pthread_t t1;
  pthread_t t2;

  /*
  turn=nondet_int();
  __VERIFIER_assume(0<=turn && turn<=1);
  __CPROVER_ASYNC_1: thr1();
  __CPROVER_ASYNC_1: thr2();
  */

  pthread_create(t1, NULL, thr1, NULL); // poet modified
  pthread_create(t2, NULL, thr2, NULL); // poet modified
  pthread_join(t1, NULL); // poet modified
  pthread_join(t2, NULL); // poet modified

  // return 0; // poet modified
}
