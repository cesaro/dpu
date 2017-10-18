# Non-deterministic poke a thread pool of workers

# xxx N=2 K=1 xxx
# dpu: por: summary: 4 max-configs, 0 SSBs, 61 events, 28.0 ev/trail
# xxx N=2 K=2 xxx
# dpu: por: summary: 14 max-configs, 0 SSBs, 229 events, 42.0 ev/trail
# xxx N=2 K=3 xxx
# dpu: por: summary: 40 max-configs, 0 SSBs, 727 events, 56.0 ev/trail
# xxx N=4 K=1 xxx
# dpu: por: summary: 8 max-configs, 0 SSBs, 143 events, 44.0 ev/trail
# xxx N=4 K=2 xxx
# dpu: por: summary: 60 max-configs, 0 SSBs, 1031 events, 62.0 ev/trail
# xxx N=4 K=3 xxx
# dpu: por: summary: 412 max-configs, 1 SSBs, 7235 events, 80.0 ev/trail
# xxx N=6 K=1 xxx
# dpu: por: summary: 12 max-configs, 0 SSBs, 249 events, 60.0 ev/trail
# xxx N=6 K=2 xxx
# dpu: por: summary: 138 max-configs, 0 SSBs, 2697 events, 82.0 ev/trail
# xxx N=6 K=3 xxx
# dpu: por: summary: 1504 max-configs, 3 SSBs, 29687 events, 104.0 ev/trail
# xxx N=8 K=1 xxx
# dpu: por: summary: 16 max-configs, 0 SSBs, 379 events, 76.0 ev/trail
# xxx N=8 K=2 xxx
# dpu: por: summary: 248 max-configs, 0 SSBs, 5515 events, 102.0 ev/trail
# xxx N=8 K=3 xxx
# dpu: por: summary: 3700 max-configs, 10 SSBs, 82819 events, 128.0 ev/trail

conf_ev[1 ]="4 61"
conf_ev[2 ]="14 229"
conf_ev[3 ]="40 727"
conf_ev[4 ]="8 143"
conf_ev[5 ]="60 1031"
conf_ev[6 ]="412 7235"
conf_ev[7 ]="12 249"
conf_ev[8 ]="138 2697"
conf_ev[9 ]="1504 29687"
conf_ev[10]="16 379"
conf_ev[11]="248 5515"
conf_ev[12]="3700 82819"
export conf_ev

MAX_N=8
MAX_K=3
N=$(seq 2 2 $MAX_N)
K=$(seq 1 $MAX_K)

for i in $N; do
   for j in $K; do
      gcc -E poke.c -D PARAM1=$i -D PARAM2=$j -o input.$i.$j.i
   done
done

# the command to test
cmd for i in $N; do \
      for j in $K; do \
         $PROG input.$i.$j.i -k4 -s 1M -vv > out.$i.$j; done; done

# the checks to perform on the output
echo N is -$N-
echo K is -$K-
test $EXITCODE = 0
ls -l out*

for i in $N; do \
   for j in $K; do \
      echo xxx N=$i K=$j xxx; grep "dpu: por: stats: " out.$i.$j; done; done

for i in $N; do \
   for j in $K; do \
      echo xxx N=$i K=$j xxx; grep "dpu: por: summary: " out.$i.$j; done; done

for i in $N; do \
   for j in $K; do \
      grep "0 SSBs" out.$i.$j; done; done

set -x; \
for i in $N; do \
   for j in $K; do \
      grep -i "ding: $((1 + $i + 2*$j)) threads created" out.$i.$j; done; done

set -x; \
k=1; \
for i in $N; do \
   for j in $K; do \
      test "$(grep "dpu: por: summary: " out.$i.$j | awk '{print $6, $10}')" = "${conf_ev[$k]}"; \
      k=$(($k + 1)); done; done

for i in $N; do \
   for j in $K; do \
      rm input.$i.$j.i; rm out.$i.$j; done; done

