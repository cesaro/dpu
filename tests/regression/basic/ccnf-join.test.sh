# Concurrent conflicts, 2N threads (1 <= N <= 11)

# xxx 1 xxx
# dpu: por: summary: 2 max-configs, 0 SSBs, 23 events, 14.0 ev/trail
# xxx 2 xxx
# dpu: por: summary: 4 max-configs, 0 SSBs, 49 events, 26.0 ev/trail
# xxx 3 xxx
# dpu: por: summary: 8 max-configs, 0 SSBs, 85 events, 38.0 ev/trail
# xxx 4 xxx
# dpu: por: summary: 16 max-configs, 0 SSBs, 141 events, 50.0 ev/trail
# xxx 5 xxx
# dpu: por: summary: 32 max-configs, 0 SSBs, 237 events, 62.0 ev/trail
# xxx 6 xxx
# dpu: por: summary: 64 max-configs, 0 SSBs, 413 events, 74.0 ev/trail
# xxx 7 xxx
# dpu: por: summary: 128 max-configs, 0 SSBs, 749 events, 86.0 ev/trail
# xxx 8 xxx
# dpu: por: summary: 256 max-configs, 0 SSBs, 1405 events, 98.0 ev/trail
# xxx 9 xxx
# dpu: por: summary: 512 max-configs, 0 SSBs, 2701 events, 110.0 ev/trail
# xxx 10 xxx
# dpu: por: summary: 1024 max-configs, 0 SSBs, 5277 events, 122.0 ev/trail
# xxx 11 xxx
# dpu: por: summary: 2048 max-configs, 0 SSBs, 10413 events, 134.0 ev/trail
# xxx 12 xxx
# dpu: por: summary: 4096 max-configs, 0 SSBs, 20669 events, 146.0 ev/trail

configs[1 ]=2
configs[2 ]=4
configs[3 ]=8
configs[4 ]=16
configs[5 ]=32
configs[6 ]=64
configs[7 ]=128
configs[8 ]=256
configs[9 ]=512
configs[10]=1024
configs[11]=2048

events[1 ]=23
events[2 ]=49
events[3 ]=85
events[4 ]=141
events[5 ]=237
events[6 ]=413
events[7 ]=749
events[8 ]=1405
events[9 ]=2701
events[10]=5277
events[11]=10413
export events configs

MAX=11
N=$(seq 1 $MAX)

for i in $N; do
   gcc -E ccnf.c -D N=$i -D JOIN=1 -o input$i.i
done

# the test
cmd for i in $N; \
   do $PROG input$i.i -s 1M -vv > out$i; done

# exactly MAX executions terminated
test $EXITCODE = 0
for i in $N; do echo "xxx $i xxx"; grep "dpu: por: summary: " out$i; done

# exactly the espected number of configurations and events
for i in $N; do test \
   "$(grep "dpu: por: summary: " out$i | awk '{print $6, $10}')" = \
   "${configs[$i]} ${events[$i]}"; done

# remove intermediate files
for i in $N; do rm input${i}.i; done
for i in $N; do rm out$i; done
