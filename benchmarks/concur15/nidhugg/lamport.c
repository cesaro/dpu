/* Adapted from: https://svn.sosy-lab.org/software/sv-benchmarks/trunk/c/pthread-atomic/lamport_true-unreach-call.c */

/* Testcase from Threader's distribution. For details see:
   http://www.model.in.tum.de/~popeea/research/threader
*/

#include <pthread.h>
#include <assert.h>

int x, y;
int b1, b2; // boolean flags
int X; // boolean variable to test mutual exclusion

void *thr1() {
  while (1) {
   __before_loop: 
    b1 = 1;
    x = 1;
    if (y != 0) {
      b1 = 0;
      busy_11: if (y != 0) { goto busy_11; } // while(y!=0) {};       
      goto __before_loop; // continue;
    }
    y = 1;
    if (x != 1) {
      b1 = 0;
      busy_12: if (b2 >= 1) { goto busy_12; } // while (b2>=1) {};
      if (y != 1) {
        busy_13: if (y != 0) { goto busy_13; } // while (y!=0) {};	
	goto __before_loop; //	continue;
      }
    }
    goto breaklbl;
  }
 breaklbl:
  // begin: critical section
  X = 0;
  if(X > 0){
      assert(0);   //  assert(X <= 0);
  }
  // end: critical section
  y = 0;
  b1 = 0;
}

void *thr2() {
  while (1) {
    __before_loop1:      
    b2 = 1;
    x = 2;
    if (y != 0) {
      b2 = 0;
      busy_21: if (y != 0) { goto busy_21; } // while(y!=0) {};       
      goto __before_loop1; // continue;
    }
    y = 2;
    if (x != 2) {
      b2 = 0;
      busy_22: if (b1 >= 1) { goto busy_22; } // while (b1>=1) {};
      if (y != 2) {
        busy_23: if (y != 0) { goto busy_23; } // while(y!=0) {};       
	goto __before_loop1; //	continue;
      }
    }
    goto breaklbl1;
  }
 breaklbl1:
  // begin: critical section
  X = 1;
  if(X < 1){
      assert(0);  //  assert(X >= 1);
  }
  // end: critical section
  y = 0;
  b2 = 0;
}

int main() {
  pthread_t t1, t2;
  pthread_create(&t1, 0, thr1, 0);
  pthread_create(&t2, 0, thr2, 0);
  pthread_join(t1, 0);
  pthread_join(t2, 0);
}
