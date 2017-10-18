# Concurrent thread creation can trigger a mismatch between pids in DPU and steroids

cmd $PROG conc-create-schedule.c -vv

test $EXITCODE = 0
grep "dpu: por: summary: 0 defects, 4 max-configs"
