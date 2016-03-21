#include <stdio.h> 
#include <pthread.h>
#include <unistd.h>

int b;

static void* helloworld( ) {
    if( 3 > b ) {
        printf("hello\n" );
    } else {
        printf( "ola\n" );
        b = 5;
    }
    return 0;
}

static void* helloworld2( ) {
    int* a;
    a = &b;
    printf( "pouet\n" );
    return 0;
}

int main( int argc, char** argv ) {
    int  i,j, k;
    void* ret;
    pthread_t t1, t2;
    static int a;
    b = 2;
    i = pthread_create( &t1, NULL, &helloworld, NULL );
    j = pthread_create( &t2, NULL, &helloworld2, NULL );

    pthread_join( t1, &ret );
    pthread_join( t2, &ret );
    
   return i;
}
