# Non-zero exit state

rm -f defects.por.yml

cmd $PROG exit1.c -vv

test $EXITCODE = 0
grep "1 defects, 1 max-configs"
test -f defects.por.yml
grep -A5 'description.*The program exited with errorcode 17' defects.por.yml
test "$(grep 'description' defects.por.yml | wc -l)" == 1
rm -f defects.por.yml
