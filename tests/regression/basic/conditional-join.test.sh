# A thread joins for another thread on some but not all replays

# the test
cmd $PROG conditional-join.c -v

test $EXITCODE = 0
grep "dpu: summary: 0 defects, 2 max-configs"
grep "dpu: stats: unfolding: 5 threads created"
