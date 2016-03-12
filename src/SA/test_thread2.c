#include <stdio.h> 
#include <pthread.h>
#include <unistd.h>

int b;

static void* helloworld( ) {
    printf("hello\n" );
}

int main( int argc, char** argv ) {
    int  i,j, k;
    void* ret;
    pthread_t t1, t2;
    static int a;
    i = pthread_create( &t1, NULL, &helloworld, NULL );
    j = pthread_create( &t2, NULL, &helloworld, NULL );

    pthread_join( t1, &ret );
    pthread_join( t2, &ret );
    
   return i;
}
