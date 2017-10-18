# m pattern

# xxx i=1 xxx
# dpu: por: summary: 2 max-configs, 0 SSBs, 21 events, 14.0 ev/trail
# xxx i=2 xxx
# dpu: por: summary: 8 max-configs, 0 SSBs, 88 events, 26.0 ev/trail
# xxx i=3 xxx
# dpu: por: summary: 48 max-configs, 0 SSBs, 491 events, 38.0 ev/trail
# xxx i=4 xxx
# dpu: por: summary: 384 max-configs, 0 SSBs, 3822 events, 50.0 ev/trail
# xxx i=5 xxx
# dpu: por: summary: 3840 max-configs, 0 SSBs, 38017 events, 62.0 ev/trail

# FIXME fill the conv_ev array
conf_ev[1]="2 21"
conf_ev[2]="8 88"
conf_ev[3]="48 491"
conf_ev[4]="384 3822"
conf_ev[5]="3840 38017"
export conf_ev

MAX=5
K=$(seq 1 $MAX)

for i in $K; do
   gcc -E mpat.c -D PARAM1=$i -o input.$i.i
done

# the command to test
cmd for i in $K; do \
      $PROG input.$i.i -k2 -s 1M -vv > out.$i; done;

# the checks to perform on the output
echo K is -$K-
test $EXITCODE = 0
ls -l out*

for i in $K; do \
   echo xxx i=$i xxx; grep "dpu: por: stats: " out.$i; done

for i in $K; do \
   echo xxx i=$i xxx; grep "dpu: por: summary: " out.$i; done

for i in $K; do \
   grep "0 SSBs" out.$i; done

for i in $K; do \
   for j in $L; do \
      grep -i " unfolding: $(($i + 4)) threads created" out.$i.$j; done; done

for i in $K; do \
   test "$(grep "dpu: por: summary: " out.$i | awk '{print $6, $10}')" = "${conf_ev[$i]}"; done

for i in $K; do \
   rm input.$i.i; rm out.$i; done
