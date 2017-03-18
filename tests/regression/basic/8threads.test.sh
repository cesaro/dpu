# Can unfold with 8 threads

gcc -E nthreads.c -D N=8 -o input.i

cmd $PROG input.i -vv

test $EXITCODE = 0
grep -i ": 1 max-configs"
grep -i " unfolding: 9 threads created"
rm input.i
