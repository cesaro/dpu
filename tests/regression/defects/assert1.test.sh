# Assertion violation

rm -f defects.yml

cmd $PROG assert1.c -vv

test $EXITCODE = 0
grep "5 defects, 24 max-configs"
grep 'Assertion .* failed.'
test -f defects.yml
grep 'description.*The program called abort' defects.yml
test "$(grep 'description' defects.yml | wc -l)" == 5
rm -f defects.yml
