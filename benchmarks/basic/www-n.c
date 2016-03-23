#include "pthread.h"    
#define N 2

int c=0;
int x=0;

void *p(){
  int l=0;
  while(c<N){
    x = 1;
    l = c;
    c = l+1;
    l =0;
  }
}

void *q(){
  int l=0;
  while(c<N){
    x = 2;
    l = c;
    c = l+1;
    l =0;
  }
}

void *r(){
  int l=0;
  while(c<N){
    x = 3;
    l = c;
    c = l+1;
    l =0;
  }
}

int main(){
    /* references to the threads */
    pthread_t p_t;
    pthread_t q_t;
    pthread_t r_t;
    
    /* create the threads and execute */
    pthread_create(&p_t, 0, p, 0);
    pthread_create(&q_t, 0, q, 0);
    pthread_create(&r_t, 0, r, 0);
    
    /* wait for the threads to finish */
    pthread_join(p_t, 0);
    pthread_join(q_t, 0);
    pthread_join(r_t, 0);

    /* show the results  */
    //    printf("x: %d\n", x);

    // return 0;
}
//int printf(const char * restrict format, ...);

//int main(){printf("Hello world\n");return 0;}
