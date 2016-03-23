#!/bin/bash

# programs with acyclic state spaces
nidhuggc -- -sc stf.c &> log/stf.log
nidhuggc -- -sc stf_bug.c &> log/stf_bug.log
nidhuggc -- -sc spin08.c &> log/spin08.log
nidhuggc -- -sc fib.c &> log/fib.log
nidhuggc -- -sc fib_bug.c &> log/fib_bug.log
nidhuggc -- -sc ccnf9.c &> log/ccnf9.log
nidhuggc -- -sc ccnf17.c &> log/ccnf17.log
nidhuggc -- -sc ccnf19.c &> log/ccnf19.log
nidhuggc -- -sc ssb.c &> log/ssb.log
nidhuggc -- -sc ssb1.c &> log/ssb1.log
nidhuggc -- -sc ssb3.c &> log/ssb3.log
nidhuggc -- -sc ssb4.c &> log/ssb4.log
nidhuggc -- -sc ssb8.c &> log/ssb8.log

# programs with non-acyclic state spaces
nidhuggc -- -sc szymanski.c &> log/szymanski.log
nidhuggc -unroll=10 -- -sc dekker.c &> log/dekker.log
nidhuggc -unroll=10 -- -sc lamport.c &> log/lamport.log
nidhuggc -unroll=10 -- -sc peterson.c &> log/peterson.log
nidhuggc -unroll=10 -- -sc pgsql.c &> log/pgsql.log
nidhuggc -unroll=10 -- -sc rwlock.c &> log/rwlock.log
nidhuggc -unroll=2 -- -sc rwlock2.c &> log/rwlock2.log
nidhuggc -unroll=5 -- -sc prodcons.c &> log/prodcons.log
nidhuggc -unroll=5 -- -sc prodcons2.c &> log/prodcons2.log
