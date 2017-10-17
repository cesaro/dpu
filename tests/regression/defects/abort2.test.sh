# Calls to abort are a defect

rm -f defects.yml

cmd $PROG abort2.c -vv

test $EXITCODE = 0
grep "1 defects, 1 max-configs"
test -f defects.yml
grep -A5 'description.*The program called abort' defects.yml
test "$(grep 'description' defects.yml | wc -l)" == 1
rm -f defects.yml
