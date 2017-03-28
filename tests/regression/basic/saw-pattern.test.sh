# Create and join a thread N times; that thread creates K concurrent threads

cmd $PROG saw-pattern.c -v

test $EXITCODE = 0
grep "dpu: summary: 0 defects, 1 max-config"
grep "dpu: stats: unfolding: 31 threads created"
grep "dpu: stats: unfolding: 4 process slots"
