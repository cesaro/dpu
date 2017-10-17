# Assertion violation

rm -f defects.yml

cmd $PROG assert2.c -vv

test $EXITCODE = 0
grep "2 defects, 1 max-configs"
grep 'Assertion .* failed.'
test -f defects.yml
grep -A5 'description.*The program called abort' defects.yml
test "$(grep 'description' defects.yml | wc -l)" == 2
rm -f defects.yml
