# Simple program with 2 threads and 1 CS

cmd $PROG ww.c -vv

test $EXITCODE = 0
grep "dpu: por: summary: 0 defects, 2 max-configs"
grep "dpu: por: stats: unfolding: 3 threads"
