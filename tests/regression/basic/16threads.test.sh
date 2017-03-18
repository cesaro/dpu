# Can unfold with 16 threads

gcc -E nthreads.c -D N=16 -o input.i

cmd $PROG input.i -vv

test $EXITCODE = 0
grep -i ": 1 max-configs"
grep -i " unfolding: 17 threads created"
rm input.i
