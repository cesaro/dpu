# Assertion violation

rm -f defects.por.yml

cmd $PROG assert2.c -vv

test $EXITCODE = 0
grep "2 defects, 1 max-configs"
grep 'Assertion .* failed.'
test -f defects.por.yml
grep -A5 'description.*The program called abort' defects.por.yml
test "$(grep 'description' defects.por.yml | wc -l)" == 2
rm -f defects.por.yml
