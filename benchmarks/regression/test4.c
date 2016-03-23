/*  
 * POET Regression Suite 
 * Test 4: 
*/
#include "pthread.h"

int x=0;
int y=0;
int z=0;

void *p(){
    y = 1;
    x = 1;
}

void *q(){
    z = 1;
    x = 2;
}

int main(){
    /* references to the threads */
    pthread_t p_t;
    pthread_t q_t;

    /* create the threads and execute */
    pthread_create(&p_t, NULL, p, NULL);
    pthread_create(&q_t, NULL, q, NULL);

    /* wait for the threads to finish */
    pthread_join(p_t, NULL);
    pthread_join(q_t, NULL);
}
