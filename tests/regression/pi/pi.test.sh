# Computing pi with N threads that mutex-lock before exiting

# seting MAX beyond 7 produces too many events in the thread 0 ...

MAX=7
N=$(seq 3 $MAX)
for i in $N; do
   gcc -E pth_pi_mutex.c -D THREADS=$i -D ITERS=2000 -o input$i.i
done

# the command to test
cmd for i in $N; do \
      $PROG input$i.i -vv > out$i; done

# the checks to perform on the output
echo $N
test $EXITCODE = 0
for i in $N; do echo xxx $i xxx; grep "dpu: por: stats: " out$i; done
for i in $N; do echo xxx $i xxx; grep "dpu: por: summary: " out$i; done

for i in $N; do grep -i " unfolding: $(($i + 1)) threads created" out$i; done
for i in $N; do rm input${i}.i; done
for i in $N; do rm out$i; done
