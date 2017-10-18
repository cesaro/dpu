# Like the saw-pattern.c but with some conflicts in the beginning

cmd $PROG saw-pattern-v.c -v

test $EXITCODE = 0
grep "dpu: por: summary: 0 defects, 3 max-config"
grep "dpu: por: stats: unfolding: 26 threads created"
grep "dpu: por: stats: unfolding: 5 process slots"
