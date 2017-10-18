# The great and only SSB3 example !!

# xxx N=2 L=2 xxx
# dpu: por: summary: 10 max-configs, 0 SSBs, 145 events, 36.8 ev/trail
# xxx N=2 L=4 xxx
# dpu: por: summary: 74 max-configs, 0 SSBs, 993 events, 47.1 ev/trail
# xxx N=2 L=5 xxx
# dpu: por: summary: 256 max-configs, 0 SSBs, 3429 events, 51.7 ev/trail
# xxx N=4 L=2 xxx
# dpu: por: summary: 14 max-configs, 0 SSBs, 218 events, 51.4 ev/trail
# xxx N=4 L=4 xxx
# dpu: por: summary: 78 max-configs, 0 SSBs, 1066 events, 62.4 ev/trail
# xxx N=4 L=5 xxx
# dpu: por: summary: 260 max-configs, 0 SSBs, 3502 events, 67.4 ev/trail
# xxx N=6 L=2 xxx
# dpu: por: summary: 18 max-configs, 0 SSBs, 303 events, 66.7 ev/trail
# xxx N=6 L=4 xxx
# dpu: por: summary: 82 max-configs, 0 SSBs, 1151 events, 77.7 ev/trail
# xxx N=6 L=5 xxx
# dpu: por: summary: 264 max-configs, 0 SSBs, 3587 events, 83.1 ev/trail
# xxx N=8 L=2 xxx
# dpu: por: summary: 22 max-configs, 0 SSBs, 400 events, 82.2 ev/trail
# xxx N=8 L=4 xxx
# dpu: por: summary: 86 max-configs, 0 SSBs, 1248 events, 93.0 ev/trail
# xxx N=8 L=5 xxx
# dpu: por: summary: 268 max-configs, 0 SSBs, 3684 events, 98.8 ev/trail
# xxx N=10 L=2 xxx
# dpu: por: summary: 26 max-configs, 0 SSBs, 509 events, 97.8 ev/trail
# xxx N=10 L=4 xxx
# dpu: por: summary: 90 max-configs, 0 SSBs, 1357 events, 108.4 ev/trail
# xxx N=10 L=5 xxx
# dpu: por: summary: 272 max-configs, 0 SSBs, 3793 events, 114.5 ev/trail
# xxx N=12 L=2 xxx
# dpu: por: summary: 30 max-configs, 0 SSBs, 630 events, 113.6 ev/trail
# xxx N=12 L=4 xxx
# dpu: por: summary: 94 max-configs, 0 SSBs, 1478 events, 123.9 ev/trail
# xxx N=12 L=5 xxx
# dpu: por: summary: 276 max-configs, 0 SSBs, 3914 events, 130.3 ev/trail
# xxx N=14 L=2 xxx
# dpu: por: summary: 34 max-configs, 0 SSBs, 763 events, 129.4 ev/trail
# xxx N=14 L=4 xxx
# dpu: por: summary: 98 max-configs, 0 SSBs, 1611 events, 139.4 ev/trail
# xxx N=14 L=5 xxx
# dpu: por: summary: 280 max-configs, 0 SSBs, 4047 events, 146.0 ev/trail
# xxx N=16 L=2 xxx
# dpu: por: summary: 38 max-configs, 0 SSBs, 908 events, 145.3 ev/trail
# xxx N=16 L=4 xxx
# dpu: por: summary: 102 max-configs, 0 SSBs, 1756 events, 155.0 ev/trail
# xxx N=16 L=5 xxx
# dpu: por: summary: 284 max-configs, 0 SSBs, 4192 events, 161.7 ev/trail

conf_ev[1 ]="10 145"
conf_ev[2 ]="74 993"
conf_ev[3 ]="256 3429"
conf_ev[4 ]="14 218"
conf_ev[5 ]="78 1066"
conf_ev[6 ]="260 3502"
conf_ev[7 ]="18 303"
conf_ev[8 ]="82 1151"
conf_ev[9 ]="264 3587"
conf_ev[10]="22 400"
conf_ev[11]="86 1248"
conf_ev[12]="268 3684"
conf_ev[13]="26 509"
conf_ev[14]="90 1357"
conf_ev[15]="272 3793"
conf_ev[16]="30 630"
conf_ev[17]="94 1478"
conf_ev[18]="276 3914"
conf_ev[19]="34 763"
conf_ev[20]="98 1611"
conf_ev[21]="280 4047"
conf_ev[22]="38 908"
conf_ev[23]="102 1756"
conf_ev[24]="284 4192"
export conf_ev

# seting MAX beyond 7 produces too many events in the thread 0 ...

MAX_N=16
N=$(seq 2 2 $MAX_N)
L="2 4 5"

for i in $N; do
   for j in $L; do
      gcc -E ssb3.c -D PARAM1=$i -D PARAM2=$j -o input.$i.$j.i
   done
done

# the command to test
cmd for i in $N; do \
      for j in $L; do \
         $PROG input.$i.$j.i -k2 -s 1M -vv > out.$i.$j; done; done

# the checks to perform on the output
echo NUM is -$N-
echo LEN is -$L-
test $EXITCODE = 0
ls -l out*

for i in $N; do \
   for j in $L; do \
      echo xxx N=$i L=$j xxx; grep "dpu: por: stats: " out.$i.$j; done; done

for i in $N; do \
   for j in $L; do \
      echo xxx N=$i L=$j xxx; grep "dpu: por: summary: " out.$i.$j; done; done

for i in $N; do \
   for j in $L; do \
      grep "0 SSBs" out.$i.$j; done; done

for i in $N; do \
   for j in $L; do \
      grep -i " unfolding: $(($i + 4)) threads created" out.$i.$j; done; done

k=1; \
for i in $N; do \
   for j in $L; do set -x; \
      test "$(grep "dpu: por: summary: " out.$i.$j | awk '{print $6, $10}')" = "${conf_ev[$k]}"; \
      k=$(($k + 1)); done; done

for i in $N; do \
   for j in $L; do \
      rm input.$i.$j.i; rm out.$i.$j; done; done
