# Calls to abort are a defect

rm -f defects.yml

cmd $PROG abort3.c -vv

test $EXITCODE = 0
grep "2 defects, 1 max-configs"
test -f defects.yml
grep -A5 'description.*The program called abort' defects.yml
test "$(grep 'description' defects.yml | wc -l)" == 2
rm -f defects.yml
