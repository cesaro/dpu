#include <pthread.h>
#include <assert.h>

int main( int argc, char** argv ) {
    pthread_t t;
    int ret;
    ret = pthread_create (&t, 0, &pthread_exit, 0);
    assert (ret == 0);
    return 0;
}
