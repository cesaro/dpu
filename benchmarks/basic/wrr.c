#include "pthread.h"
    
int x=0;

void *p(){
    x = 1;
}

void *q(){
    int m = x;
}

void *r(){
    int n = x;
}

int main(){
    /* references to the threads */
    pthread_t p_t;
    pthread_t q_t;
    pthread_t r_t;
    
    /* create the threads and execute */
    pthread_create(p_t, NULL, p, NULL);
    pthread_create(q_t, NULL, q, NULL);
    pthread_create(r_t, NULL, r, NULL);
    
    /* wait for the threads to finish */
    pthread_join(p_t, NULL);
    pthread_join(q_t, NULL);
    pthread_join(r_t, NULL);

    /* show the results  */
    //printf("x: %d\n", x);

    //return 0;
}
//int printf(const char * restrict format, ...);

