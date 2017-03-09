# Displays help

# $PROG program
# $EXITCODE exitcode
# $TEST path to this file

cmd $PROG --help

test $EXITCODE = 0
grep -i Usage
grep -i -- --help
grep -i -- --gdb
grep -i default
