#!/bin/bash

# programs with acyclic state spaces
{ echo "poet without cutoffs:"; time poet explore -i=stf.c; } &> log/stf.log
{ echo "poet cutoffs:"; time poet explore -i=stf.c -c=1; } &>> log/stf.log
{ echo "poet without cutoffs:"; time poet explore -i=stf_bug.c; } &> log/stf_bug.log
{ echo "poet cutoffs:"; time poet explore -i=stf_bug.c -c=1; } &>> log/stf_bug.log
{ echo "poet without cutoffs:"; time poet explore -i=spin08.c; } &> log/spin08.log
{ echo "poet cutoffs:"; time poet explore -i=spin08.c -c=1; } &>> log/spin08.log
{ echo "poet without cutoffs:"; time poet explore -i=fib.c; } &> log/fib.log
{ echo "poet cutoffs:"; time poet explore -i=fib.c -c=1; } &>> log/fib.log
{ echo "poet without cutoffs:"; time poet explore -i=fib_bug.c; } &> log/fib_bug.log
{ echo "poet cutoffs:"; time poet explore -i=fib_bug.c -c=1; } &>> log/fib_bug.log
{ echo "poet without cutoffs:"; time poet explore -i=ccnf9.c; } &> log/ccnf9.log
{ echo "poet cutoffs:"; time poet explore -i=ccnf9.c -c=1; } &>> log/ccnf9.log
{ echo "poet without cutoffs:"; time poet explore -i=ccnf17.c; } &> log/ccnf17.log
{ echo "poet cutoffs:"; time poet explore -i=ccnf17.c -c=1; } &>> log/ccnf17.log
{ echo "poet without cutoffs:"; time poet explore -i=ccnf19.c; } &> log/ccnf19.log
{ echo "poet cutoffs:"; time poet explore -i=ccnf19.c -c=1; } &>> log/ccnf19.log
{ echo "poet without cutoffs:"; time poet explore -i=ssb.c; } &> log/ssb.log
{ echo "poet cutoffs:"; time poet explore -i=ssb.c -c=1; } &>> log/ssb.log
{ echo "poet without cutoffs:"; time poet explore -i=ssb1.c; } &> log/ssb1.log
{ echo "poet cutoffs:"; time poet explore -i=ssb1.c -c=1; } &>> log/ssb1.log
{ echo "poet without cutoffs:"; time poet explore -i=ssb3.c; } &> log/ssb3.log
{ echo "poet cutoffs:"; time poet explore -i=ssb3.c -c=1; } &>> log/ssb3.log
{ echo "poet without cutoffs:"; time poet explore -i=ssb4.c; } &> log/ssb4.log
{ echo "poet cutoffs:"; time poet explore -i=ssb4.c -c=1; } &>> log/ssb4.log
{ echo "poet without cutoffs:"; time poet explore -i=ssb8.c; } &> log/ssb8.log
{ echo "poet cutoffs:"; time poet explore -i=ssb8.c -c=1; } &>> log/ssb8.log

# programs with non-acyclic state spaces
{ echo "poet cutoffs:"; time poet explore -i=szymanski.c -c=1; } &> log/szymanski.log
{ echo "poet cutoffs:"; time poet explore -i=dekker.c -c=1; } &> log/dekker.log
{ echo "poet cutoffs:"; time poet explore -i=lamport.c -c=1; } &> log/lamport.log
{ echo "poet cutoffs:"; time poet explore -i=peterson.c -c=1; } &> log/peterson.log
{ echo "poet cutoffs:"; time poet explore -i=pgsql.c -c=1; } &> log/pgsql.log
{ echo "poet cutoffs:"; time poet explore -i=rwlock.c -c=1; } &> log/rwlock.log
{ echo "poet cutoffs:"; time poet explore -i=rwlock2.c -c=1; } &> log/rwlock2.log
{ echo "poet cutoffs:"; time poet explore -i=prodcons.c -c=1; } &> log/prodcons.log
{ echo "poet cutoffs:"; time poet explore -i=prodcons2.c -c=1; } &> log/prodcons2.log
