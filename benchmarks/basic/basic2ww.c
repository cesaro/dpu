
#include <pthread.h>
#include <stdio.h>

int x=0;

void * p (void * arg){
   while (1)
   {
      x=1;
      x=1;
      x=1;
      x=1;
   }
   return 0;
}

void * q (void * arg){
   while (1)
   {
      x=2;
      x=2;
      x=2;
      x=2;
   }
   return 0;
}

void * r (void * arg){
   while (1)
   {
      x=2;
      x=2;
      x=2;
      x=2;
   }
   return 0;
}

int main (int argc, char ** argv)
{
   /* references to the threads */
   pthread_t p_t;
   pthread_t q_t;
   pthread_t r_t;
   
   /* create the threads and execute */
   pthread_create (&p_t, 0, p, 0);
   pthread_create (&q_t, 0, q, 0);
   pthread_create (&r_t, 0, r, 0);

   /* wait for the threads to finish */
   pthread_join (p_t, 0);
   pthread_join (q_t, 0);
   pthread_join (r_t, 0);

   /* show the results  */
   printf("x: %d\n", x);

   return 0;
}
