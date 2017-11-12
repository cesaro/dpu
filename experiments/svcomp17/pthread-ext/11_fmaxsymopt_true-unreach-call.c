extern void __VERIFIER_assume(int);
extern void __VERIFIER_error() __attribute__ ((__noreturn__));
extern int __VERIFIER_nondet_int();

#include <pthread.h>

#define assume(e) __VERIFIER_assume(e)
#define assert_nl(e) { if(!(e)) { goto ERROR; } }
#define assert(e) { if(!(e)) { ERROR: __VERIFIER_error();(void)0; } }

#define WORKPERTHREAD 2
#define THREADSMAX 3
volatile int max = 0x80000000, m = 0;

void __VERIFIER_atomic_acquire()
{
	assume(m==0);
	m = 1;
}

void __VERIFIER_atomic_release()
{
	assume(m==1);
	m = 0;
}

int storage[WORKPERTHREAD*THREADSMAX];

inline void findMax(int offset){

	int i;
	int e;
	int my_max = 0x80000000;

	for(i = offset; i < offset+WORKPERTHREAD; i++) {
#ifndef NOBUG
		e = storage[i];
#else
    e = rand();
#endif

		if(e > my_max) {
			my_max = e;
		}
		assert_nl(e <= my_max);
	}

	__VERIFIER_atomic_acquire();
	{
		if(my_max > max) {
			max = my_max;
		}
	}
	__VERIFIER_atomic_release();

	assert(my_max <= max);
}

void* thr1(void* arg) {
	int offset=__VERIFIER_nondet_int();

	assume(offset % WORKPERTHREAD == 0 && offset >= 0 && offset < WORKPERTHREAD*THREADSMAX);
	//assume(offset < WORKPERTHREAD && offset >= 0 && offset < WORKPERTHREAD*THREADSMAX);

	findMax(offset);

  return 0;
}

int main(){
  pthread_t t;

	while(1) { pthread_create(&t, 0, thr1, 0); }
}

