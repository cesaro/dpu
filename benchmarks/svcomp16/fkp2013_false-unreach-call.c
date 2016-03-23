// Source: Azadeh Farzan, Zachary Kincaid, Andreas Podelski: "Inductive Data
// Flow Graphs", POPL 2013

// adapted for POET

#include "../pthread.h"

volatile int x = 0;

void* thr1(void* arg) {
    //__VERIFIER_assert(x < N);
	 if (x >= 3) __poet_fail ();
}

void* thr2_1(void* arg) {
    int t;
    t = x;
    x = t + 1;
}


void* thr2_2(void* arg) {
    int t;
    t = x;
    x = t + 1;
}

void* thr2_3(void* arg) {
    int t;
    t = x;
    x = t + 1;
}

void main(int argc, char* argv[]) {
    pthread_t t1, t2, t3, t4;

    pthread_create(t1, NULL, thr1, NULL);
    pthread_create(t2, NULL, thr2_1, NULL);
    pthread_create(t3, NULL, thr2_2, NULL);
    pthread_create(t4, NULL, thr2_3, NULL);
    pthread_join(t1,NULL);
    pthread_join(t2,NULL);
    pthread_join(t3,NULL);
    pthread_join(t4,NULL);
}

