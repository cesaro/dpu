# Can unfold with 24 threads

gcc -E nthreads.c -D N=24 -o input.i

cmd $PROG input.i -vv --mem 64M --stack 1M

test $EXITCODE = 0
grep -i ": 1 max-configs"
grep -i " unfolding: 25 threads created"
rm input.i
