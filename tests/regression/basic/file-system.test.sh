# [FG05] file-system example, 2^(N-13) executions

# xxx 1 xxx
# dpu: por: summary: 1 max-configs, 0 SSBs, 10 events, 10.0 ev/trail
# xxx 2 xxx
# dpu: por: summary: 1 max-configs, 0 SSBs, 18 events, 18.0 ev/trail
# xxx 3 xxx
# dpu: por: summary: 1 max-configs, 0 SSBs, 26 events, 26.0 ev/trail
# xxx 4 xxx
# dpu: por: summary: 1 max-configs, 0 SSBs, 34 events, 34.0 ev/trail
# xxx 5 xxx
# dpu: por: summary: 1 max-configs, 0 SSBs, 42 events, 42.0 ev/trail
# xxx 6 xxx
# dpu: por: summary: 1 max-configs, 0 SSBs, 50 events, 50.0 ev/trail
# xxx 7 xxx
# dpu: por: summary: 1 max-configs, 0 SSBs, 58 events, 58.0 ev/trail
# xxx 8 xxx
# dpu: por: summary: 1 max-configs, 0 SSBs, 66 events, 66.0 ev/trail
# xxx 9 xxx
# dpu: por: summary: 1 max-configs, 0 SSBs, 74 events, 74.0 ev/trail
# xxx 10 xxx
# dpu: por: summary: 1 max-configs, 0 SSBs, 82 events, 82.0 ev/trail
# xxx 11 xxx
# dpu: por: summary: 1 max-configs, 0 SSBs, 90 events, 90.0 ev/trail
# xxx 12 xxx
# dpu: por: summary: 1 max-configs, 0 SSBs, 98 events, 98.0 ev/trail
# xxx 13 xxx
# dpu: por: summary: 1 max-configs, 0 SSBs, 106 events, 106.0 ev/trail
# xxx 14 xxx
# dpu: por: summary: 2 max-configs, 0 SSBs, 141 events, 116.0 ev/trail
# xxx 15 xxx
# dpu: por: summary: 4 max-configs, 0 SSBs, 192 events, 126.0 ev/trail
# xxx 16 xxx
# dpu: por: summary: 8 max-configs, 0 SSBs, 275 events, 136.0 ev/trail
# xxx 17 xxx
# dpu: por: summary: 16 max-configs, 0 SSBs, 422 events, 146.0 ev/trail
# xxx 18 xxx
# dpu: por: summary: 32 max-configs, 0 SSBs, 697 events, 156.0 ev/trail
# xxx 19 xxx
# dpu: por: summary: 64 max-configs, 0 SSBs, 1228 events, 166.0 ev/trail
# xxx 20 xxx
# dpu: por: summary: 128 max-configs, 0 SSBs, 2271 events, 176.0 ev/trail
# xxx 21 xxx
# dpu: por: summary: 256 max-configs, 0 SSBs, 4338 events, 186.0 ev/trail

configs[1 ]=1
configs[2 ]=1
configs[3 ]=1
configs[4 ]=1
configs[5 ]=1
configs[6 ]=1
configs[7 ]=1
configs[8 ]=1
configs[9 ]=1
configs[10]=1
configs[11]=1
configs[12]=1
configs[13]=1
configs[14]=2
configs[15]=4
configs[16]=8
configs[17]=16
configs[18]=32
configs[19]=64
configs[20]=128
configs[21]=256

events[1 ]=10
events[2 ]=18
events[3 ]=26
events[4 ]=34
events[5 ]=42
events[6 ]=50
events[7 ]=58
events[8 ]=66
events[9 ]=74
events[10]=82
events[11]=90
events[12]=98
events[13]=106
events[14]=141
events[15]=192
events[16]=275
events[17]=422
events[18]=697
events[19]=1228
events[20]=2271
events[21]=4338

MAX=21
N=$(seq 1 $MAX)

export events configs # arrays need to be exported manually

# generate
for i in $N; do
   gcc -E file-system.c -D N=$i -o input$i.i
done

# xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

cmd for i in $N; do $PROG input$i.i -vv -s 1M > out$i; done

# exactly MAX executions terminated
test $EXITCODE = 0
for i in $N; do echo "xxx $i xxx"; grep "dpu: por: stats: " out$i; done
for i in $N; do echo "xxx $i xxx"; grep "dpu: por: summary: " out$i; done

# exactly the espected number of configurations and events
for i in $N; do test \
   "$(grep "dpu: por: summary: " out$i | awk '{print $6, $10}')" = \
   "${configs[$i]} ${events[$i]}"; done
for i in $N; do grep "dpu: por: stats: unfolding: $(($i + 1)) threads created" out$i; done

# remove intermediate files
for i in $N; do rm input${i}.i; done
for i in $N; do rm out$i; done
