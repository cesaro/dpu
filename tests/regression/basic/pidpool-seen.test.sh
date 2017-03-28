# Reusing pids after join

# the test
cmd $PROG pidpool-seen.c -v

test $EXITCODE = 0
grep "dpu: summary: 0 defects, 3 max-configs"
grep "dpu: stats: unfolding: 3 threads created"
grep "dpu: stats: unfolding: 2 process slots"
grep "dpu: stats: por: 0 SSBs"
