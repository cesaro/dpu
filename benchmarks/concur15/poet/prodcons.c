/*  */

#include "pthread.h"

#define MAX   6
#define MIN   0

int buf=0;
pthread_mutex_t l1; 

void *prod1()
{
    int aux=0;
    while(1){
        pthread_mutex_lock(l1);
        if(buf < MAX){
            aux=buf;
            buf=aux+1;
            aux=0;
        }
        pthread_mutex_unlock(l1);         
    }
}


void *prod2()
{
    int aux=0;
    while(1){
        pthread_mutex_lock(l1);
        if(buf < MAX){
            aux=buf;
            buf=aux+2;
            aux=0;
        }
        pthread_mutex_unlock(l1);         
    }
}


void *cons()
{
    int aux=0;
    while(1){
       pthread_mutex_lock(l1);
       if(buf > MIN){
           aux=buf;
           buf=aux-1;
//           aux=0;
       }
       pthread_mutex_unlock(l1); 
    }
}

int main()
{

  pthread_t id1; 
  pthread_t id2;
  pthread_t id3;

  pthread_mutex_init(l1, NULL); 
  
  pthread_create(id1, NULL, prod1, NULL);
  pthread_create(id2, NULL, prod2, NULL);
  pthread_create(id3, NULL, cons, NULL);

  pthread_join(id1, NULL);
  pthread_join(id2, NULL);
  pthread_join(id3, NULL);
}
