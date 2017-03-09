# N threads concurrently create new threads; ensure that tids get correctly assigned

# xxx 1 xxx
# dpu: summary: 1 max-configs, 0 SSBs, 9 events, 9.0 ev/trail
# xxx 2 xxx
# dpu: summary: 1 max-configs, 0 SSBs, 15 events, 15.0 ev/trail
# xxx 3 xxx
# dpu: summary: 1 max-configs, 0 SSBs, 22 events, 22.0 ev/trail
# xxx 4 xxx
# dpu: summary: 1 max-configs, 0 SSBs, 28 events, 28.0 ev/trail
# xxx 5 xxx
# dpu: summary: 1 max-configs, 0 SSBs, 35 events, 35.0 ev/trail
# xxx 6 xxx
# dpu: summary: 1 max-configs, 0 SSBs, 41 events, 41.0 ev/trail
# xxx 7 xxx
# dpu: summary: 1 max-configs, 0 SSBs, 48 events, 48.0 ev/trail
# xxx 8 xxx
# dpu: summary: 1 max-configs, 0 SSBs, 54 events, 54.0 ev/trail
# xxx 9 xxx
# dpu: summary: 1 max-configs, 0 SSBs, 61 events, 61.0 ev/trail
# xxx 10 xxx
# dpu: summary: 1 max-configs, 0 SSBs, 67 events, 67.0 ev/trail
# xxx 11 xxx
# dpu: summary: 1 max-configs, 0 SSBs, 74 events, 74.0 ev/trail
# xxx 12 xxx
# dpu: summary: 1 max-configs, 0 SSBs, 80 events, 80.0 ev/trail
# xxx 13 xxx
# dpu: summary: 1 max-configs, 0 SSBs, 87 events, 87.0 ev/trail

MAX=13
N=$(seq 1 $MAX)

events[1]=9
events[2]=15
events[3]=22
events[4]=28
events[5]=35
events[6]=41
events[7]=48
events[8]=54
events[9]=61
events[10]=67
events[11]=74
events[12]=80
events[13]=87
export events
for i in $N; do
   gcc -E concurrent-create.c -D N=$i -o input$i.i
done

# the test
cmd for i in $N; do $PROG input$i.i -s 1M -vv > out$i; cat out$i; done

# exactly MAX executions terminated
test $EXITCODE = 0
for i in $N; do echo "xxx $i xxx"; grep "dpu: summary: " out$i; done

# exactly the espected number of events
for i in $N; do test \
   "$(grep "dpu: summary: " out$i | awk '{print $7}')" = ${events[$i]}; done

# remove intermediate files
for i in $N; do rm input${i}.i; done
for i in $N; do rm out$i; done
