# 44 different combinations of ld/st in 11 thread locations

# programs 1 to 23 are safe
# programs 24 to 44 are racy
SAFE=$(seq 1 23)
RACY=$(seq 24 44)

# ld/st in same thread are not racy
gcc -E races1.c -D LD1=1 -D ST1=1 -o input1.i
gcc -E races1.c -D LD1=1 -D ST2=1 -o input2.i
gcc -E races1.c -D LD1=1 -D ST3=1 -o input3.i
gcc -E races1.c -D ST1=1 -D ST2=1 -o input4.i
gcc -E races1.c -D ST1=1 -D ST3=1 -o input5.i

# ld in != threads are not racy
gcc -E races1.c -D LD1=1 -D LD4=1 -o input6.i
gcc -E races1.c -D LD1=1 -D LD5=1 -o input7.i
gcc -E races1.c -D LD1=1 -D LD6=1 -o input8.i

gcc -E races1.c -D LD2=1 -D LD4=1 -o input9.i
gcc -E races1.c -D LD2=1 -D LD5=1 -o input10.i
gcc -E races1.c -D LD2=1 -D LD6=1 -o input11.i

gcc -E races1.c -D LD3=1 -D LD4=1 -o input12.i
gcc -E races1.c -D LD3=1 -D LD5=1 -o input13.i
gcc -E races1.c -D LD3=1 -D LD6=1 -o input14.i

# ld/st before creating thread is not racy
gcc -E races1.c -D ST7=1 -D LD1=1 -o input15.i
gcc -E races1.c -D ST7=1 -D LD3=1 -o input16.i
gcc -E races1.c -D ST7=1 -D LD4=1 -o input17.i

# ld/st after joining thread is not racy
gcc -E races1.c -D ST10=1 -D LD4=1 -o input18.i
gcc -E races1.c -D ST10=1 -D LD5=1 -o input19.i
gcc -E races1.c -D ST10=1 -D LD6=1 -o input20.i

# variables in the CS are not racy
gcc -E races1.c -D ST2=1 -D ST5=1 -o input21.i
gcc -E races1.c -D LD2=1 -D ST5=1 -o input22.i
gcc -E races1.c -D LD2=1 -D LD5=1 -o input23.i

# st/st, ld/st, st/ld for concurrent events are races
gcc -E races1.c -D ST1=1 -D ST4=1 -o input24.i
gcc -E races1.c -D LD1=1 -D ST4=1 -o input25.i
gcc -E races1.c -D ST1=1 -D LD4=1 -o input26.i

# all possible races between between action inside CS the main
gcc -E races1.c -D ST2=1 -D LD8=1 -o input27.i
gcc -E races1.c -D ST2=1 -D LD9=1 -o input28.i
gcc -E races1.c -D ST2=1 -D LD10=1 -o input29.i

# all possible races between thread 1 and 2
gcc -E races1.c -D ST1=1 -D LD4=1 -o input30.i
gcc -E races1.c -D ST1=1 -D LD5=1 -o input31.i
gcc -E races1.c -D ST1=1 -D LD6=1 -o input32.i

gcc -E races1.c -D ST2=1 -D LD4=1 -o input33.i
gcc -E races1.c -D ST2=1 -D LD6=1 -o input34.i

gcc -E races1.c -D ST3=1 -D LD4=1 -o input35.i
gcc -E races1.c -D ST3=1 -D LD5=1 -o input36.i
gcc -E races1.c -D ST3=1 -D LD6=1 -o input37.i

# race between action after create and anywhere in the thread
gcc -E races1.c -D ST9=1 -D ST4=1 -o input38.i
gcc -E races1.c -D ST9=1 -D ST5=1 -o input39.i
gcc -E races1.c -D ST9=1 -D ST6=1 -o input40.i

gcc -E races1.c -D ST9=1 -D ST1=1 -o input41.i
gcc -E races1.c -D ST9=1 -D ST2=1 -o input42.i
gcc -E races1.c -D ST9=1 -D ST3=1 -o input43.i

# three actions, has race
gcc -E races1.c -D ST11=1 -D LD8=1 -D ST3=1 -o input44.i

cmd for i in $SAFE $RACY; do \
      $PROG input$i.i -a dr -v --drfreq 100 > out$i; done

test $EXITCODE = 0
for i in $SAFE; do echo xxx $i xxx; grep "dpu: dr: result: " out$i; done
for i in $RACY; do echo xxx $i xxx; grep "dpu: dr: result: " out$i; done
for i in $SAFE $RACY; do grep "dpu: por: stats: unfolding: 2 max-configs" out$i; done
for i in $SAFE $RACY; do grep "dpu: por: stats: unfolding: 3 threads created" out$i; done

for i in $SAFE; do grep "dpu: dr: result: .*NO data race found" out$i; done
for i in $RACY; do grep "dpu: dr: result: .*data race FOUND" out$i; done

rm defects.{dr,por}.yml
for i in $SAFE $RACY; do rm input$i.i out$i; done
