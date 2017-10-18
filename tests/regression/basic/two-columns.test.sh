# Two threads sequentially acquire N times one mutex

# N  max confs (according to nidhugg)
# 1  2
# 2  6
# 3  20
# 4  70
# 5  252
# 6  924
# 7  3432
# 8  12870

configs[1]=2
configs[2]=6
configs[3]=20
configs[4]=70
configs[5]=252
configs[6]=924
configs[7]=3432
configs[8]=12870
export configs

# initialization
MAX=8
N=$(seq 1 $MAX)
for i in $N; do
   gcc -E two-columns.c -D N=$i -o input$i.i
done

# the command to test
cmd for i in $N; do \
      $PROG input$i.i -vv > out$i; done

# exactly MAX executions terminated
echo $N
test $EXITCODE = 0
for i in $N; do echo xxx $i xxx; grep "dpu: por: summary: " out$i; done
for i in $N; do echo xxx $i xxx; grep "dpu: por: stats: " out$i; done


# exactly the espected number of configurations, events, and threads
for i in $N; do set -x; test \
   "$(grep "dpu: por: summary: " out$i | awk '{print $6}')" = \
   "${configs[$i]}"; done
for i in $N; do grep " unfolding: 2 threads created" out$i; done

# remove intermediate files
for i in $N; do rm input$i.i; done
for i in $N; do rm out$i; done
