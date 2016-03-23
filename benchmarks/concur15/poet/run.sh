#!/bin/bash

# programs with acyclic state spaces
{ echo "poet without cutoffs:"; time poet explore stf.c; } &> log/stf.log
{ echo "poet cutoffs:"; time poet explore stf.c -c=1; } &>> log/stf.log
{ echo "poet without cutoffs:"; time poet explore stf_bug.c; } &> log/stf_bug.log
{ echo "poet cutoffs:"; time poet explore stf_bug.c -c=1; } &>> log/stf_bug.log
{ echo "poet without cutoffs:"; time poet explore spin08.c; } &> log/spin08.log
{ echo "poet cutoffs:"; time poet explore spin08.c -c=1; } &>> log/spin08.log
{ echo "poet without cutoffs:"; time poet explore fib.c; } &> log/fib.log
{ echo "poet cutoffs:"; time poet explore fib.c -c=1; } &>> log/fib.log
{ echo "poet without cutoffs:"; time poet explore fib_bug.c; } &> log/fib_bug.log
{ echo "poet cutoffs:"; time poet explore fib_bug.c -c=1; } &>> log/fib_bug.log
{ echo "poet without cutoffs:"; time poet explore ccnf9.c; } &> log/ccnf9.log
{ echo "poet cutoffs:"; time poet explore ccnf9.c -c=1; } &>> log/ccnf9.log
{ echo "poet without cutoffs:"; time poet explore ccnf17.c; } &> log/ccnf17.log
{ echo "poet cutoffs:"; time poet explore ccnf17.c -c=1; } &>> log/ccnf17.log
{ echo "poet without cutoffs:"; time poet explore ccnf19.c; } &> log/ccnf19.log
{ echo "poet cutoffs:"; time poet explore ccnf19.c -c=1; } &>> log/ccnf19.log
{ echo "poet without cutoffs:"; time poet explore ssb.c; } &> log/ssb.log
{ echo "poet cutoffs:"; time poet explore ssb.c -c=1; } &>> log/ssb.log
{ echo "poet without cutoffs:"; time poet explore ssb1.c; } &> log/ssb1.log
{ echo "poet cutoffs:"; time poet explore ssb1.c -c=1; } &>> log/ssb1.log
{ echo "poet without cutoffs:"; time poet explore ssb3.c; } &> log/ssb3.log
{ echo "poet cutoffs:"; time poet explore ssb3.c -c=1; } &>> log/ssb3.log
{ echo "poet without cutoffs:"; time poet explore ssb4.c; } &> log/ssb4.log
{ echo "poet cutoffs:"; time poet explore ssb4.c -c=1; } &>> log/ssb4.log
{ echo "poet without cutoffs:"; time poet explore ssb8.c; } &> log/ssb8.log
{ echo "poet cutoffs:"; time poet explore ssb8.c -c=1; } &>> log/ssb8.log

# programs with non-acyclic state spaces
{ echo "poet cutoffs:"; time poet explore szymanski.c -c=1; } &> log/szymanski.log
{ echo "poet cutoffs:"; time poet explore dekker.c -c=1; } &> log/dekker.log
{ echo "poet cutoffs:"; time poet explore lamport.c -c=1; } &> log/lamport.log
{ echo "poet cutoffs:"; time poet explore peterson.c -c=1; } &> log/peterson.log
{ echo "poet cutoffs:"; time poet explore pgsql.c -c=1; } &> log/pgsql.log
{ echo "poet cutoffs:"; time poet explore rwlock.c -c=1; } &> log/rwlock.log
{ echo "poet cutoffs:"; time poet explore rwlock2.c -c=1; } &> log/rwlock2.log
{ echo "poet cutoffs:"; time poet explore prodcons.c -c=1; } &> log/prodcons.log
{ echo "poet cutoffs:"; time poet explore prodcons2.c -c=1; } &> log/prodcons2.log
