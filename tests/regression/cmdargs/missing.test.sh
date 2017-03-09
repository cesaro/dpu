# Dpu signals an error on a missing file

# $PROG program
# $EXITCODE exitcode
# $TEST path to this file

cmd $PROG -v 

test $EXITCODE != 0
#grep -i usage
test $(wc -l) -ge 1 # at least one line of output
