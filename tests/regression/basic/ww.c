
#include <pthread.h>
#include <stdio.h>
#include <assert.h>

int x=0;
pthread_mutex_t l;

void * p (void * arg){
    int y = 0;
    pthread_mutex_lock (&l);
    y = x;
    x = 1;
    printf ("p: y %d\n", y);
    pthread_mutex_unlock (&l);

    return 0;
}

void * q (void * arg){
    pthread_mutex_lock (&l);
    x = 2;
    printf ("q: x %d\n", x);
    pthread_mutex_unlock (&l);

	 return 0;
}

int main ()
{
    /* references to the threads */
    pthread_t p_t;
    pthread_t q_t;
    
    pthread_mutex_init (&l, 0);

    /* create the threads and execute */
    pthread_create (&p_t, 0, p, 0);
    pthread_create (&q_t, 0, q, 0);

    /* wait for the threads to finish */
    pthread_join (p_t, 0);
    pthread_join (q_t, 0);

    assert (x != 3);

    /* show the results  */
    printf("x: %d\n", x);

    return 0;
}
