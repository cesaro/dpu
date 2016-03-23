#include "pthread.h"
    
int x=0;

void *p(){
   // while(1){
      x=1;
      x=1;
      x=1;
      x=1;
   // }
}

void *q(){
   // while(1){
        x=2;
        x=2;
        x=2;
        x=2;
   // }
}

void *r(){
   // while(1){
        x=3;
        x=3;
        x=3;
        x=3;
   // }
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
}
