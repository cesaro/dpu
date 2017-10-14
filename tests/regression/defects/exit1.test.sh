# Non-zero exit state

rm -f defects.yml

cmd $PROG exit1.c -vv

test $EXITCODE = 0
grep "1 defects, 1 max-configs"
test -f defects.yml
grep 'description.*The program exited with errorcode 17' defects.yml
test "$(grep 'description' defects.yml | wc -l)" == 1
rm -f defects.yml
