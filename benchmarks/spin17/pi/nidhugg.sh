#!/bin/sh

/usr/local/bin/nidhuggc \
    --nidhugg=/usr/local/bin/nidhugg \
    -extfun-no-race=printf \
    -extfun-no-race=write \
    -extfun-no-race=exit \
    -extfun-no-race=atoi \
       $*
