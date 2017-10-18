# S servers exchange R request

#xxx S=3 R=2 xxx
#dpu: por: summary: 59 max-configs, 0 SSBs, 834 events, 48.0 ev/trail
#xxx S=4 R=2 xxx
#dpu: por: summary: 94 max-configs, 0 SSBs, 1445 events, 58.0 ev/trail
#xxx S=5 R=2 xxx
#dpu: por: summary: 137 max-configs, 0 SSBs, 2288 events, 68.0 ev/trail
#xxx S=6 R=2 xxx
#dpu: por: summary: 188 max-configs, 0 SSBs, 3399 events, 78.0 ev/trail
#xxx S=7 R=2 xxx
#dpu: por: summary: 247 max-configs, 0 SSBs, 4814 events, 88.0 ev/trail
#xxx S=8 R=2 xxx
#dpu: por: summary: 314 max-configs, 0 SSBs, 6569 events, 98.0 ev/trail (confirmed up to here with nidhugg)
#xxx S=9 R=2 xxx
#dpu: por: summary: 389 max-configs, 0 SSBs, 8700 events, 108.0 ev/trail
#xxx S=10 R=2 xxx
#dpu: por: summary: 472 max-configs, 0 SSBs, 11243 events, 118.0 ev/trail
#xxx S=11 R=2 xxx
#dpu: por: summary: 563 max-configs, 0 SSBs, 14234 events, 128.0 ev/trail
#xxx S=12 R=2 xxx
#dpu: por: summary: 662 max-configs, 0 SSBs, 17709 events, 138.0 ev/trail
#xxx S=13 R=2 xxx
#dpu: por: summary: 769 max-configs, 0 SSBs, 21704 events, 148.0 ev/trail
#xxx S=14 R=2 xxx
#dpu: por: summary: 884 max-configs, 0 SSBs, 26255 events, 158.0 ev/trail
#xxx S=15 R=2 xxx
#dpu: por: summary: 1007 max-configs, 0 SSBs, 31398 events, 168.0 ev/trail

conf_ev[1 ]="59 834"
conf_ev[2 ]="94 1445"
conf_ev[3 ]="137 2288"
conf_ev[4 ]="188 3399"
conf_ev[5 ]="247 4814"
conf_ev[6 ]="314 6569"
conf_ev[7 ]="389 8700"
conf_ev[8 ]="472 11243"
conf_ev[9 ]="563 14234"
conf_ev[10]="662 17709"
conf_ev[11]="769 21704"
conf_ev[12]="884 26255"
conf_ev[13]="1007 31398"
export conf_ev

MAX=10
S=$(seq 3 $MAX)
R="2" # with 3, 4, or more we hit the limit of processe

for i in $S; do
   for j in $R; do
      gcc -E dispatcher.c -D PARAM1=$i -D PARAM2=$j -o input.$i.$j.i
   done
done

# the command to test
cmd for i in $S; do \
      for j in $R; do \
         $PROG input.$i.$j.i -k3 -s 1M -vv > out.$i.$j; done; done

# the checks to perform on the output
echo S is -$S-
echo R is -$R-
test $EXITCODE = 0
ls -l out*

for i in $S; do \
   for j in $R; do \
      echo xxx S=$i R=$j xxx; grep "dpu: por: stats: " out.$i.$j; done; done

for i in $S; do \
   for j in $R; do \
      echo xxx S=$i R=$j xxx; grep "dpu: por: summary: " out.$i.$j; done; done

for i in $S; do \
   for j in $R; do \
      grep "0 SSBs" out.$i.$j; done; done

set -x; \
for i in $S; do \
   for j in $R; do \
      grep -i "ding: $(($i + $j + 1)) threads created" out.$i.$j; done; done

k=1; \
for i in $S; do \
   for j in $R; do set -x; \
      test "$(grep "dpu: por: summary: " out.$i.$j | awk '{print $6, $10}')" = "${conf_ev[$k]}"; \
      k=$(($k + 1)); done; done

#for i in $S; do \
#   for j in $R; do \
#      mynidhugg input.$i.$j.i; done; done

for i in $S; do \
   for j in $R; do \
      rm input.$i.$j.i; rm out.$i.$j; done; done
