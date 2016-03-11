/* #include <stdio.h> */
int main( int argc, char** argv ) {
    int  i,j;
    if( argc >= 2 ) {
        i = 1;
    } else {
        i = 2;
    }
    for( j = 0 ; j < 3 ; j++ ) {
        printf( "Hello %d\n", i );
    }
    return i;
}
