# N threads enter CS and create new threads

# xxx 1 xxx
# dpu: summary: 1 max-configs, 0 SSBs, 10 events, 11.0 ev/trail
# xxx 2 xxx
# dpu: summary: 2 max-configs, 0 SSBs, 30 events, 20.0 ev/trail
# xxx 3 xxx
# dpu: summary: 6 max-configs, 0 SSBs, 98 events, 29.0 ev/trail

MAX=3
N=$(seq 1 $MAX)

configs[1]=1
configs[2]=2
configs[3]=6

events[1]=10
events[2]=30
events[3]=98
export events configs

for i in $N; do
   gcc -E conflict-create.c -D N=$i -o input$i.i
done

# the test
cmd for i in $N; do $PROG input$i.i -s 1M -vv > out$i; cat out$i; done

# exactly MAX executions terminated
test $EXITCODE = 0
for i in $N; do echo "xxx $i xxx"; grep "dpu: summary: " out$i; done

# exactly the espected number of configurations and events
for i in $N; do test \
   "$(grep "dpu: summary: " out$i | awk '{print $3, $7}')" = \
   "${configs[$i]} ${events[$i]}"; done

# remove intermediate files
for i in $N; do rm input${i}.i; done
for i in $N; do rm out$i; done
