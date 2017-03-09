# S servers exchange R request

# xxx S=3 R=2 xxx
# dpu: summary: 59 max-configs, 0 SSBs, 834 events, 48.0 ev/trail
# xxx S=6 R=2 xxx
# dpu: summary: 188 max-configs, 0 SSBs, 3399 events, 78.0 ev/trail
# xxx S=9 R=2 xxx
# dpu: summary: 389 max-configs, 0 SSBs, 8700 events, 108.0 ev/trail
# xxx S=12 R=2 xxx
# dpu: summary: 662 max-configs, 0 SSBs, 17709 events, 138.0 ev/trail
# xxx S=15 R=2 xxx
# dpu: summary: 1007 max-configs, 0 SSBs, 31398 events, 168.0 ev/trail

# FIXME fill the conv_ev array
conf_ev[1]="59 834"
conf_ev[2]=""
export conf_ev

MAX=15
S=$(seq 3 3 $MAX)
R="2" # with 3, 4, or more we hit the limit of processe

for i in $S; do
   for j in $R; do
      gcc -E dispatcher.c -D PARAM1=$i -D PARAM2=$j -o input.$i.$j.i
   done
done

# the command to test
cmd for i in $S; do \
      for j in $R; do \
         $PROG input.$i.$j.i -a2 -s 1M -vv > out.$i.$j; done; done

# the checks to perform on the output
echo S is -$S-
echo R is -$R-
test $EXITCODE = 0
ls -l out*

for i in $S; do \
   for j in $R; do \
      echo xxx S=$i R=$j xxx; grep "dpu: stats: " out.$i.$j; done; done

for i in $S; do \
   for j in $R; do \
      echo xxx S=$i R=$j xxx; grep "dpu: summary: " out.$i.$j; done; done

for i in $S; do \
   for j in $R; do \
      grep "0 SSBs" out.$i.$j; done; done

for i in $S; do \
   for j in $R; do \
      grep -i " unfolding: $(($i + 4)) threads created" out.$i.$j; done; done

k=1; \
for i in $S; do \
   for j in $R; do set -x; \
      test "$(grep "dpu: summary: " out.$i.$j | awk '{print $3, $7}')" = "${conf_ev[$k]}"; \
      k=$(($k + 1)); done; done

for i in $S; do \
   for j in $R; do \
      rm input.$i.$j.i; rm out.$i.$j; done; done
