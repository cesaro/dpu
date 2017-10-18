# Reusing pids after join

# the test
cmd $PROG pidpool-seen.c -v

test $EXITCODE = 0
grep "dpu: por: summary: 0 defects, 3 max-configs"
grep "dpu: por: stats: unfolding: 3 threads created"
grep "dpu: por: stats: unfolding: 2 process slots"
grep "dpu: por: stats: por: 0 SSBs"
