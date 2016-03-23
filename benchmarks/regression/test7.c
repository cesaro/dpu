/*  
 * POET Regression Suite 
 * Test 1: Independent writes to different global variables
*/
#include "pthread.h"

int x=0;
int y=0;

void *p(){
    int i =0;
    while(i<5){i=i+1;}
    x = 2;
}

int main(){
    /* references to the threads */
    pthread_t p_t;

    /* create the threads and execute */
    pthread_create(&p_t, NULL, p, NULL);
    x = 1;
    int i=0;
    while(i<5){i=i+1;}
     
}
