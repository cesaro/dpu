# Calls to abort are a defect

rm -f defects.por.yml

cmd $PROG abort1.c -vv

test $EXITCODE = 0
grep "1 defects, 1 max-configs"
test -f defects.por.yml
grep -A5 'description.*The program called abort' defects.por.yml
test "$(grep 'description' defects.por.yml | wc -l)" == 1
rm -f defects.por.yml
