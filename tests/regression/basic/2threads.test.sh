# Can unfold with 2 threads

gcc -E nthreads.c -D N=2 -o input.i

cmd $PROG input.i -vv

test $EXITCODE = 0
grep -i ": 1 max-configs"
grep -i " unfolding: 3 threads created"
rm input.i
