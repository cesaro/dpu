# Can unfold with 4 threads

gcc -E nthreads.c -D N=4 -o input.i

cmd $PROG input.i -vv

test $EXITCODE = 0
grep -i ": 1 max-configs"
grep -i " unfolding: 5 threads created"
rm input.i
