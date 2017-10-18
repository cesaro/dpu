# Unfolding with N threads and N! executions

# dpu: por: summary: 1 max-configs, 0 SSBs, 8 events, 8.0 ev/trail
# dpu: por: summary: 2 max-configs, 0 SSBs, 23 events, 14.0 ev/trail
# dpu: por: summary: 6 max-configs, 0 SSBs, 75 events, 20.0 ev/trail
# dpu: por: summary: 24 max-configs, 0 SSBs, 311 events, 26.0 ev/trail
# dpu: por: summary: 120 max-configs, 0 SSBs, 1623 events, 32.0 ev/trail
# dpu: por: summary: 720 max-configs, 0 SSBs, 10221 events, 38.0 ev/trail
# dpu: por: summary: 5040 max-configs, 0 SSBs, 75113 events, 44.0 ev/trail

MAX=5
N=$(seq 1 $MAX)

configs[1]=1
configs[2]=2
configs[3]=6
configs[4]=24
configs[5]=120
configs[6]=720
configs[7]=5040

events[1]=8
events[2]=23
events[3]=75
events[4]=311
events[5]=1623
events[6]=10221
events[7]=75113
export events configs # arrays need to be exported manually

# generate
for i in $N; do
   gcc -E factorial-conflicts.c -D N=$i -o input$i.i
done

# xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

cmd for i in $N; do $PROG input$i.i -vv > out$i; cat out$i; done

# exactly MAX executions terminated
test $EXITCODE = 0
for i in $N; do echo "xxx $i xxx"; grep "dpu: por: stats: " out$i; done

# exactly the espected number of configurations and events
for i in $N; do test \
   "$(grep "dpu: por: summary: " out$i | awk '{print $6, $10}')" = \
   "${configs[$i]} ${events[$i]}"; done

# remove intermediate files
for i in $N; do rm input${i}.i; done
for i in $N; do rm out$i; done
