# Threads that create other threads

# out1:dpu: por: summary: 1 max-configs, 0 SSBs, 6 events, 6.0 ev/trail
# out2:dpu: por: summary: 1 max-configs, 0 SSBs, 10 events, 10.0 ev/trail
# out3:dpu: por: summary: 1 max-configs, 0 SSBs, 14 events, 14.0 ev/trail
# out4:dpu: por: summary: 1 max-configs, 0 SSBs, 18 events, 18.0 ev/trail
# out5:dpu: por: summary: 1 max-configs, 0 SSBs, 22 events, 22.0 ev/trail
# out6:dpu: por: summary: 1 max-configs, 0 SSBs, 26 events, 26.0 ev/trail
# out7:dpu: por: summary: 1 max-configs, 0 SSBs, 30 events, 30.0 ev/trail
# out8:dpu: por: summary: 1 max-configs, 0 SSBs, 34 events, 34.0 ev/trail
# out9:dpu: por: summary: 1 max-configs, 0 SSBs, 38 events, 38.0 ev/trail
# out10:dpu: por: summary: 1 max-configs, 0 SSBs, 42 events, 42.0 ev/trail
# out11:dpu: por: summary: 1 max-configs, 0 SSBs, 46 events, 46.0 ev/trail
# out12:dpu: por: summary: 1 max-configs, 0 SSBs, 50 events, 50.0 ev/trail
# out13:dpu: por: summary: 1 max-configs, 0 SSBs, 54 events, 54.0 ev/trail
# out14:dpu: por: summary: 1 max-configs, 0 SSBs, 58 events, 58.0 ev/trail
# out15:dpu: por: summary: 1 max-configs, 0 SSBs, 62 events, 62.0 ev/trail
# out16:dpu: por: summary: 1 max-configs, 0 SSBs, 66 events, 66.0 ev/trail
# out17:dpu: por: summary: 1 max-configs, 0 SSBs, 70 events, 70.0 ev/trail
# out18:dpu: por: summary: 1 max-configs, 0 SSBs, 74 events, 74.0 ev/trail
# out19:dpu: por: summary: 1 max-configs, 0 SSBs, 78 events, 78.0 ev/trail
# out20:dpu: por: summary: 1 max-configs, 0 SSBs, 82 events, 82.0 ev/trail

# initialization
MAX=20
N=$(seq 1 $MAX)
for i in $N; do
   gcc -E thread-creates.c -D N=$i -o input$i.i
done

# the command to test
cmd \
   for i in $N; do \
      echo xxxxxxxxxxxxxxxxxxxxxxxxx; \
      echo i $i; \
      $PROG input$i.i -s 1M | tee out$i; done


# the checks to perform on the output
test $EXITCODE = 0
for i in $N; do grep summary out$i; done
for i in $N; do grep -i ": 1 max-configs" out$i; done
for i in $N; do grep -i " unfolding: $(($i + 1)) threads created" out$i; done
for i in $N; do rm input${i}.i; done
for i in $N; do rm out$i; done
