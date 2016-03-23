/*  
 * POET Regression Suite 
 * Test: SSB (Sleep set blocking executions) 
*/
#include <pthread.h>
#define N 4

int x=0;
int y=0;
int z=0;

void *p(){
    x = 1;
}

void *q(){
    y = 1;
}

void *r(){
    if (y == 0){
      z=1;
    }
}

void *s(){
    int aux=0;
    while(y<N){
      if (z == 1){
        x=2;
      }
      aux=y;
      y=aux+1;
      aux=0;
    }
}

int main(){
    /* references to the threads */
    pthread_t p_t;
    pthread_t q_t;
    pthread_t r_t;
    pthread_t s_t;

    /* create the threads and execute */
    pthread_create(&p_t, 0, p, 0);
    pthread_create(&q_t, 0, q, 0);
    pthread_create(&r_t, 0, r, 0);
    pthread_create(&s_t, 0, s, 0);

    /* wait for the threads to finish */
    pthread_join(p_t, 0);
    pthread_join(q_t, 0);
    pthread_join(r_t, 0);
    pthread_join(s_t, 0);
}
