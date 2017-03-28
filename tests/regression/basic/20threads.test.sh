# Can unfold with 20 threads

gcc -E nthreads.c -D N=20 -o input.i

cmd $PROG input.i -vv

test $EXITCODE = 0
grep "summary: 0 defects, 1 max-configs"
grep " unfolding: 21 threads created"
rm input.i
