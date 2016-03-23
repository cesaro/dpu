typedef unsigned long int pthread_t;
typedef union {
            char __size[56]; long int __align;
        } pthread_attr_t;

int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void*), void *arg);
int pthread_join(pthread_t thread, void **arg);
int printf(const char * restrict format, ...);
    
int x=0;

void *p(){
    x = 1;
    return NULL;
}

void *q(){
    x = 2;
    return NULL;
}

int main(){
    /* references to the threads */
    pthread_t p_t;
    pthread_t q_t;

    /* create the threads and execute */
    pthread_create(&p_t, NULL, p, NULL);
    pthread_create(&q_t, NULL, q, NULL);

    /* wait for the threads to finish */
    pthread_join(p_t, NULL);
    pthread_join(q_t, NULL);


    /* show the results  */
    printf("x: %d\n", x);

    return 0;
}
//int printf(const char * restrict format, ...);

//int main(){printf("Hello world\n");return 0;}
