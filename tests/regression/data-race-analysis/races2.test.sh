# Race depends on threads running 1 specific interleaving

N="2 3"

cmd for i in $N; do \
      $PROG races2.c -D N=$i -a dr -v --drfreq 100 > out$i; done

test $EXITCODE = 0
for i in $N; do echo xxx $i xxx; grep "dpu: dr: result: " out$i; done
for i in $N; do grep "dpu: dr: result: .*data race FOUND" out$i; done

rm defects.dr.yml
for i in $N; do rm out$i; done
