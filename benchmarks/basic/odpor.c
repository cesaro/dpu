
#include <pthread.h>
#include <stdio.h>

int x, y, z = 0;

void *p (void * arg)
{
    x = 1;
    return 0;
}

void *q (void * arg)
{
    y = 1;
    return 0;
}

void *r (void * arg)
{
    int m = y;
    if (m == 0){
        z = 1;
    }
    return 0;
}

void *s (void * arg)
{
    int n = z;
    int l = y;
    if (n == 1){
        if(l == 0){
            x = 2;
        }
    }
    return 0;
}

int main(){
    /* references to the threads */
    pthread_t p_t, q_t, r_t, s_t;

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


    /* show the results  */
    printf("x: %d, y: %d, z: %d\n", x, y, z);

    return 0;
}
