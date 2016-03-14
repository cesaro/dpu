#include <stdio.h> 
#include <pthread.h>
#include <unistd.h>

int b;
pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;

static void* helloworld( ) {
    pthread_mutex_lock( &mut );
    if( 3 > b ) {
        pthread_mutex_unlock( &mut );
        printf("hello\n" );
    } else {
        printf( "ola\n" );
        b = 5;
        pthread_mutex_unlock( &mut );
    }
    return 0;
}

int main( int argc, char** argv ) {
    int  i,j, k;
    void* ret;
    pthread_t t1, t2;
    static int a;
    b = 2;
    i = pthread_create( &t1, NULL, &helloworld, NULL );
    j = pthread_create( &t2, NULL, &helloworld, NULL );

    pthread_join( t1, &ret );
    pthread_join( t2, &ret );
    
   return i;
}
