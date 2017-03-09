# Dpu signals an error on wrong options

# $PROG program
# $EXITCODE exitcode
# $TEST path to this file

cmd $PROG --fjsdkl || $PROG --jjqkl || $PROG -4

test $EXITCODE != 0
#grep -i usage
test $(wc -l) -ge 1 # at least one line of output
