typedef unsigned long int pthread_t;
typedef union {
            char __size[56]; long int __align;
        } pthread_attr_t;

int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void*), void *arg);
int pthread_join(pthread_t thread, void **arg);
int printf(const char * restrict format, ...);
    
int x=0;
int y=0;
int z = 0;

void *p(){
    x = 1;
    return NULL;
}

void *q(){
    y = 1;
    return NULL;
}

void *r(){
    int m = y;
    if (m == 0){
        z = 1;
    }
    return NULL;
}

void *s(){
    int n = z;
    int l = y;
    if (n == 1){
        if(l == 0){
            x = 2;
        }
    }
    return NULL;
}

int main(){
    /* references to the threads */
    pthread_t p_t;
    pthread_t q_t;
    pthread_t r_t;
    pthread_t s_t;

    /* create the threads and execute */
    pthread_create(&p_t, NULL, p, NULL);
    pthread_create(&q_t, NULL, q, NULL);
    pthread_create(&r_t, NULL, r, NULL);
    pthread_create(&s_t, NULL, s, NULL);

    /* wait for the threads to finish */
    pthread_join(p_t, NULL);
    pthread_join(q_t, NULL);
    pthread_join(r_t, NULL);
    pthread_join(s_t, NULL);


    /* show the results  */
    printf("x: %d, y: %d, z: %d\n", x, y, z);

    return 0;
}
//int printf(const char * restrict format, ...);

//int main(){printf("Hello world\n");return 0;}
