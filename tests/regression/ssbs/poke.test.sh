# poke

# xxx N=2 K=1 xxx
# dpu: summary: 4 max-configs, 0 SSBs, 61 events, 28.0 ev/trail
# xxx N=2 K=2 xxx
# dpu: summary: 14 max-configs, 0 SSBs, 229 events, 42.0 ev/trail
# xxx N=4 K=1 xxx
# dpu: summary: 8 max-configs, 0 SSBs, 143 events, 44.0 ev/trail
# xxx N=4 K=2 xxx
# dpu: summary: 60 max-configs, 1 SSBs, 1031 events, 61.9 ev/trail
# xxx N=6 K=1 xxx
# dpu: summary: 12 max-configs, 0 SSBs, 249 events, 60.0 ev/trail
# xxx N=6 K=2 xxx
# dpu: summary: 138 max-configs, 18 SSBs, 2697 events, 81.0 ev/trail
# xxx N=8 K=1 xxx
# dpu: summary: 16 max-configs, 0 SSBs, 379 events, 76.0 ev/trail
# xxx N=10 K=1 xxx
# dpu: summary: 20 max-configs, 0 SSBs, 533 events, 92.0 ev/trail
# xxx N=12 K=1 xxx
# dpu: summary: 24 max-configs, 0 SSBs, 711 events, 108.0 ev/trail
# xxx N=14 K=1 xxx
# dpu: summary: 28 max-configs, 0 SSBs, 913 events, 124.0 ev/trail
# xxx N=16 K=1 xxx
# dpu: summary: 32 max-configs, 0 SSBs, 1139 events, 140.0 ev/trail

# if N >= 10 we need to set K to 1, otherwise we excede the limit of processes
# in DPU

conf_ev[1]=""
export conf_ev

MAX_N=16
N=$(seq 2 2 $MAX_N)
K="1 2"

for i in $N; do
   for j in $K; do
      [ $i -ge 8 -a $j = 2 ] && continue; \
      gcc -E poke.c -D PARAM1=$i -D PARAM2=$j -o input.$i.$j.i
   done
done

# the command to test
cmd for i in $N; do \
      for j in $K; do \
         [ $i -ge 8 -a $j = 2 ] && continue; \
         $PROG input.$i.$j.i -a3 -s 1M -vv > out.$i.$j; done; done

# the checks to perform on the output
echo N is -$N-
echo K is -$K-
test $EXITCODE = 0
ls -l out*

for i in $N; do \
   for j in $K; do \
      [ $i -ge 8 -a $j = 2 ] && continue; \
      echo xxx N=$i K=$j xxx; grep "dpu: stats: " out.$i.$j; done; done

for i in $N; do \
   for j in $K; do \
      [ $i -ge 8 -a $j = 2 ] && continue; \
      echo xxx N=$i K=$j xxx; grep "dpu: summary: " out.$i.$j; done; done

for i in $N; do \
   for j in $K; do \
      [ $i -ge 8 -a $j = 2 ] && continue; \
      grep "0 SSBs" out.$i.$j; done; done

set -x; \
for i in $N; do \
   for j in $K; do \
      [ $i -ge 8 -a $j = 2 ] && continue; \
      grep -i " threads created ($(($i + 2*$j)) actual threads" out.$i.$j; done; done
      # this will fail, we need something more advanced

k=1; \
for i in $N; do \
   for j in $K; do set -x; \
      [ $i -ge 8 -a $j = 2 ] && continue; \
      test "$(grep "dpu: summary: " out.$i.$j | awk '{print $3, $7}')" = "${conf_ev[$k]}"; \
      k=$(($k + 1)); done; done

for i in $N; do \
   for j in $K; do \
      [ $i -ge 8 -a $j = 2 ] && continue; \
      rm input.$i.$j.i; rm out.$i.$j; done; done
