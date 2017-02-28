#!/bin/sh

/usr/local/bin/nidhuggc \
    --nidhugg=/usr/local/bin/nidhugg \
    -extfun-no-race=printf \
    -extfun-no-race=write \
    -extfun-no-race=exit \
    -extfun-no-race=atoi \
    -extfun-no-race=pthread_mutexattr_init \
    -extfun-no-race=pthread_yield \
    -extfun-no-race=pthread_detach \
    -extfun-no-race=pthread_mutexattr_settype \
    -extfun-no-race=pthread_mutexattr_init \
       $*
