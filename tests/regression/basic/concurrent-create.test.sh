# N threads concurrently create new threads; ensure that tids get correctly assigned

# xxx 1 xxx
# dpu: por: summary: 6 max-configs, 0 SSBs, 59 events, 21.0 ev/trail
# xxx 2 xxx
# dpu: por: summary: 6 max-configs, 0 SSBs, 66 events, 28.0 ev/trail
# xxx 3 xxx
# dpu: por: summary: 6 max-configs, 0 SSBs, 74 events, 36.0 ev/trail
# xxx 4 xxx
# dpu: por: summary: 6 max-configs, 0 SSBs, 81 events, 43.0 ev/trail
# xxx 5 xxx
# dpu: por: summary: 6 max-configs, 0 SSBs, 89 events, 51.0 ev/trail
# xxx 6 xxx
# dpu: por: summary: 6 max-configs, 0 SSBs, 96 events, 58.0 ev/trail
# xxx 7 xxx
# dpu: por: summary: 6 max-configs, 0 SSBs, 104 events, 66.0 ev/trail
# xxx 8 xxx
# dpu: por: summary: 6 max-configs, 0 SSBs, 111 events, 73.0 ev/trail
# xxx 9 xxx
# dpu: por: summary: 6 max-configs, 0 SSBs, 119 events, 81.0 ev/trail
# xxx 10 xxx
# dpu: por: summary: 6 max-configs, 0 SSBs, 126 events, 88.0 ev/trail
# xxx 11 xxx
# dpu: por: summary: 6 max-configs, 0 SSBs, 134 events, 96.0 ev/trail
# xxx 12 xxx
# dpu: por: summary: 6 max-configs, 0 SSBs, 141 events, 103.0 ev/trail
# xxx 13 xxx
# dpu: por: summary: 0 defects, 6 max-configs, 0 SSBs, 149 events, 111.0 ev/trail

MAX=13
N=$(seq 1 $MAX)

events[1 ]=59
events[2 ]=66
events[3 ]=74
events[4 ]=81
events[5 ]=89
events[6 ]=96
events[7 ]=104
events[8 ]=111
events[9 ]=119
events[10]=126
events[11]=134
events[12]=141
events[13]=149
export events

for i in $N; do
   gcc -E concurrent-create.c -D N=$i -o input$i.i
done

# the test
cmd for i in $N; do $PROG input$i.i -s 1M -vv > out$i; cat out$i; done

# exactly MAX executions terminated
test $EXITCODE = 0
for i in $N; do echo "xxx $i xxx"; grep "dpu: por: summary: " out$i; done

# exactly the espected number of events
for i in $N; do test \
   "$(grep "dpu: por: summary: " out$i | awk '{print $10}')" = ${events[$i]}; done

# remove intermediate files
   for i in $N; do rm input${i}.i; done
for i in $N; do rm out$i; done
