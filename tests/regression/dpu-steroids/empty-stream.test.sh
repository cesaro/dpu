# Steroids returns an empty stream of actions

rm -f defects.yml

cmd $PROG empty-stream.c -vv

test $EXITCODE = 0
# should be 1 event, but ok due to limitations on steroids
grep "dpu: summary: 1 defects, 1 max-configs.*2 events"
grep "dpu: stats: unfolding: 1 threads created"
rm defects.yml
