# Testing various capabilities of steroids

# the command to test
cmd $PROG conc-factorial-reuse.c -v

# the checks to perform on the output
test $EXITCODE = 0
grep "dpu: por: summary: 0 defects, 120 max-configs"
grep "dpu: por: stats: unfolding: 26 threads created"

