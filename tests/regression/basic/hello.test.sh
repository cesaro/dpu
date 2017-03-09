# Testing various capabilities of steroids

# the command to test
cmd $PROG hello.c -vv

# the checks to perform on the output
test $EXITCODE = 0
grep "dpu: summary: 1 max-configs"
grep "dpu: stats: unfolding: 1 threads created"

