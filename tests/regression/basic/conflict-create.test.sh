# N threads enter CS and then create new threads

# xxx 1 xxx
# dpu: por: summary: 1 max-configs, 0 SSBs, 10 events, 10.0 ev/trail
# xxx 2 xxx
# dpu: por: summary: 2 max-configs, 0 SSBs, 30 events, 18.0 ev/trail
# xxx 3 xxx
# dpu: por: summary: 6 max-configs, 0 SSBs, 98 events, 26.0 ev/trail
# xxx 4 xxx
# dpu: por: summary: 24 max-configs, 0 SSBs, 394 events, 34.0 ev/trail
# xxx 5 xxx
# dpu: por: summary: 120 max-configs, 0 SSBs, 1962 events, 42.0 ev/trail
# xxx 6 xxx
# dpu: por: summary: 720 max-configs, 0 SSBs, 11750 events, 50.0 ev/trail
# xxx 7 xxx
# dpu: por: summary: 5040 max-configs, 0 SSBs, 82210 events, 58.0 ev/trail

MAX=7
N=$(seq 1 $MAX)

conf_ev[1]="1 10"
conf_ev[2]="2 30"
conf_ev[3]="6 98"
conf_ev[4]="24 394"
conf_ev[5]="120 1962"
conf_ev[6]="720 11750"
conf_ev[7]="5040 82210"
export conf_ev

for i in $N; do
   gcc -E conflict-create.c -D N=$i -o input$i.i
done

# the test
cmd for i in $N; do $PROG input$i.i -s 1M -vv > out$i; cat out$i; done

# exactly MAX executions terminated
test $EXITCODE = 0
for i in $N; do echo "xxx $i xxx"; grep "dpu: por: summary: " out$i; done

# exactly the espected number of configurations and events
for i in $N; do test \
   "$(grep "dpu: por: summary: " out$i | awk '{print $6, $10}')" = \
   "${conf_ev[$i]}"; done

# remove intermediate files
for i in $N; do rm input${i}.i; done
for i in $N; do rm out$i; done
