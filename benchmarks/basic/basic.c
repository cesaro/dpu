#include "pthread.h"
    
int x=0;
int y=nondet(0,10);
int k=nondet(0,3);

//int z=0;

void *p(){
    if(k == 0) {
      x = nondet(0,5);  
    }  else {
      x = nondet(1,1);
    }
    y = 0;
    //int i = y;
    //if(x >= 10) {
    //  __poet_fail();
    //}
    
//    z = 1;
//    y = 1;
}

/*
void *q(){
    k = 2;
//    z = 2;
//    x = 2;
}
*/
int main(){
    /* references to the threads */
    pthread_t p_t;
//    pthread_t q_t;
    
    /* create the threads and execute */
    pthread_create(p_t, NULL, p, NULL);
//    pthread_create(q_t, NULL, q, NULL);
    
    /* wait for the threads to finish */
    pthread_join(p_t, NULL);
//    pthread_join(q_t, NULL);
}
//[9,8,7,6,4,3,2,1,0]
