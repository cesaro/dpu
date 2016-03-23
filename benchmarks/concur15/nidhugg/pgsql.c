/* Adapted from PGSQL benchmark from http://link.springer.com/chapter/10.1007%2F978-3-642-37036-6_28 */

/* BOUND 10 */

//#include <stdbool.h>
#include <pthread.h>
#include <assert.h>

//void __VERIFIER_assume(int);

int latch1 = 1;
int flag1  = 1;
int latch2 = 0;
int flag2  = 0;

int __unbuffered_tmp2 = 0;

void* worker_1()
{
  while(1) {
    // __VERIFIER_assume(latch1);
  L1: if(latch1 != 1) goto L1;
    //assert(!latch1 || flag1);
    if(latch1){
      if(!flag1){
        assert(0);
      }
    }

    latch1 = 0;
    if(flag1) {
      flag1 = 0;
      flag2 = 1;
      latch2 = 1;
    }
  }
}

void* worker_2()
{
  while(1) {
    //    __VERIFIER_assume(latch2);
  L2: if(!latch2) goto L2;
    //    assert(!latch2 || flag2);
    if(latch2){
      if(!flag2){
        assert(0);
      }
    }
    latch2 = 0;
    if(flag2) {
      flag2 = 0;
      flag1 = 1;
      latch1 = 1;
    }
  }
}

int main() {
  pthread_t t1;
  pthread_t t2;
  pthread_create(&t1, 0, worker_1, 0);
  pthread_create(&t2, 0, worker_2, 0);
}
