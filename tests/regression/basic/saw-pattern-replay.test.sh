# Like the saw-pattern.c but with some conflicts in the end

cmd $PROG saw-pattern-replay.c -v

test $EXITCODE = 0
grep "dpu: summary: 3 max-config"
grep "dpu: stats: unfolding: 18 threads created"
grep "dpu: stats: unfolding: 5 process slots"
