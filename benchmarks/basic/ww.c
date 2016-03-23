#include "pthread.h"

int x=0;
pthread_mutex_t l;

void *p(){
    int y=0;
    pthread_mutex_lock(l);
    y = x;
    x = 1;
    pthread_mutex_unlock(l);
    //return NULL;
}

void *q(){
    pthread_mutex_lock(l);
    x = 2;
    pthread_mutex_unlock(l);
    //return NULL;
}

int main(){
    /* references to the threads */
    pthread_t p_t;
    pthread_t q_t;
    
    pthread_mutex_init(l, NULL);

    /* create the threads and execute */
    pthread_create(p_t, NULL, p, NULL);
    pthread_create(q_t, NULL, q, NULL);

    /* wait for the threads to finish */
    pthread_join(p_t, NULL);
    pthread_join(q_t, NULL);


    /* show the results  */
    //printf("x: %d\n", x);

    //return 0;
}
