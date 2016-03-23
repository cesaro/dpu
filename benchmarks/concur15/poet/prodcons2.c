/*  */

#include "pthread.h"

#define MAX   6
#define MIN   0

int buf_1=0;
int buf_2=0;
pthread_mutex_t l1;
pthread_mutex_t l2;

void *prod1()
{
    int aux=0;
    while(1){
        pthread_mutex_lock(l1);
        if(buf_1 < MAX){
            aux=buf_1;
            buf_1=aux+1;
            aux=0;
        }
        pthread_mutex_unlock(l1);         
    }
}


void *prod2()
{
    int aux=0;
    while(1){
        pthread_mutex_lock(l2);
        if(buf_2 < MAX){
            aux=buf_2;
            buf_2=aux+1;
            aux=0;
        }
        pthread_mutex_unlock(l2);         
    }
}


void *cons()
{
    int aux=0;
    while(1){
       pthread_mutex_lock(l1);
       if(buf_1 > MIN){
           aux=buf_1;
           buf_1=aux-1;
//           aux=0;
       }
       pthread_mutex_unlock(l1); 
       pthread_mutex_lock(l2);
       if(buf_2 > MIN){
           aux=buf_2;
           buf_2=aux-1;
//           aux=0;
       }
       pthread_mutex_unlock(l2); 
    }
}

int main()
{

  pthread_t id1; 
  pthread_t id2;
  pthread_t id3;

  pthread_mutex_init(l1, NULL); 
  pthread_mutex_init(l2, NULL); 
  
  pthread_create(id1, NULL, prod1, NULL);
  pthread_create(id2, NULL, prod2, NULL);
  pthread_create(id3, NULL, cons, NULL);

  pthread_join(id1, NULL);
  pthread_join(id2, NULL);
  pthread_join(id3, NULL);
}
