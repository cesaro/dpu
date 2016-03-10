/* #include <stdio.h> */
int main( int argc, char** argv ) {
    int  i;
    if( argc >= 2 ) {
        i = 1;
    } else {
        i = 2;
    }
    printf( "Hello %d\n", i );
    return i;
}
