//extern void __VERIFIER_error() __attribute__ ((__noreturn__));

#include "pthread.h"

#define SIZE  128
#define MAX   4
#define NUM_THREADS  2

int tab[SIZE];
pthread_mutex_t  cas_mutex[SIZE];

pthread_t  tids[NUM_THREADS];

/* int cas(int tab[SIZE], int h, int val, int new_val) */
/* { */
/*   int ret_val = 0; */
/*   pthread_mutex_lock(&cas_mutex[h]); */
  
 
/*   if ( tab[h] == val ) { */
/*     tab[h] = new_val; */
/*     ret_val = 1; */
/*   } */

/*   pthread_mutex_unlock(&cas_mutex[h]); */

  
/*   return ret_val; */
/* }  */



void * t1()
{
  int tid1=0; //*((int *)arg);
  int m1=0;
  int w1=0;
  int h1=0;
  
  while(1){
    if ( m1 < MAX ){
      m1 = m1 + 1;
      w1 = m1 * 11 + tid1;
    }
    else{
      return 0;
    }
    
    h1 = (w1 * 7) % SIZE;
    
    if (h1<0){
      //ERROR: // TODO: put assert here? 
      return 0;
    }
    
    int flag1 = 1;
    while (1) // cas(table, h, 0, w) == 0)
    {
      pthread_mutex_lock(&cas_mutex[h1]);
      if (tab[h1] == 0) {
	    tab[h1] = w1;
    	flag1 = 0;
      }
      pthread_mutex_unlock(&cas_mutex[h1]);
      
      if(!flag1)
	    h1 = (h1+1) % SIZE;
      else
	    break;
    }
  }
  return 0;
}

void * t2()
{
  int tid2=1; //*((int *)arg);
  int m2 = 0;
  int w2 = 0;
  int h2 = 0;
  
  while(1){
    if ( m2 < MAX ){
      m2 = m2+1;
      w2 = m2 * 11 + tid2;
    }
    else{
      return 0;
    }
    
    h2 = (w2 * 7) % SIZE;
    
    if (h2<0){
      //ERROR: // TODO: put assert here? 
      return 0;
    }
    
    int flag2 = 1;
    while (1) // cas(table, h, 0, w) == 0)
    {
      pthread_mutex_lock(&cas_mutex[h2]);
      if (tab[h2] == 0) {
          tab[h2] = w2;
          flag2 = 0;
      }
      pthread_mutex_unlock(&cas_mutex[h2]);
      
      if(!flag2)
          h2 = (h2+1) % SIZE;
      else
	      break;
    }
  }
  return 0;
}

int main()
{
  int i=0;

  while (i < SIZE)
  {
      pthread_mutex_init(&cas_mutex[i], 0);
      i+=1;
  }

  pthread_create(&tids[0], 0, t1, 0);
  pthread_create(&tids[1], 0, t2, 0);

  pthread_join(tids[0], 0);
  pthread_join(tids[1], 0);

}
