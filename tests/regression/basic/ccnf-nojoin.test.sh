# Concurrent conflicts, 2N threads (1 <= N <= 11), no join

# xxx 1 xxx
# dpu: por: summary: 2 max-configs, 0 SSBs, 18 events, 12.0 ev/trail
# xxx 2 xxx
# dpu: por: summary: 4 max-configs, 0 SSBs, 34 events, 22.0 ev/trail
# xxx 3 xxx
# dpu: por: summary: 8 max-configs, 0 SSBs, 50 events, 32.0 ev/trail
# xxx 4 xxx
# dpu: por: summary: 16 max-configs, 0 SSBs, 66 events, 42.0 ev/trail
# xxx 5 xxx
# dpu: por: summary: 32 max-configs, 0 SSBs, 82 events, 52.0 ev/trail
# xxx 6 xxx
# dpu: por: summary: 64 max-configs, 0 SSBs, 98 events, 62.0 ev/trail
# xxx 7 xxx
# dpu: por: summary: 128 max-configs, 0 SSBs, 114 events, 72.0 ev/trail
# xxx 8 xxx
# dpu: por: summary: 256 max-configs, 0 SSBs, 130 events, 82.0 ev/trail
# xxx 9 xxx
# dpu: por: summary: 512 max-configs, 0 SSBs, 146 events, 92.0 ev/trail
# xxx 10 xxx
# dpu: por: summary: 1024 max-configs, 0 SSBs, 162 events, 102.0 ev/trail
# xxx 11 xxx
# dpu: por: summary: 2048 max-configs, 0 SSBs, 178 events, 112.0 ev/trail

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

events[1 ]=18
events[2 ]=34
events[3 ]=50
events[4 ]=66
events[5 ]=82
events[6 ]=98
events[7 ]=114
events[8 ]=130
events[9 ]=146
events[10]=162
events[11]=178
export events configs

MAX=11
N=$(seq 1 $MAX)

for i in $N; do
   gcc -E ccnf.c -D N=$i -D JOIN=0 -o input$i.i
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
