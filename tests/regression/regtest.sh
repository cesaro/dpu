#!/bin/bash

# global variables (value passed accross functions):
PROGNAME=regtest
RM=
FAIL=0
PASS=0
SKIP=0
I=
LOGG=
LOG=
OUT=
ERR=
PRE=
TEST=
TESTPATH=
EXITCODE=
WALLTIME=
DESCR=
THIS_PASSED=
THIS_SKIPPED=

usage ()
{
   echo "Usage: $PROGNAME PROGRAM [REGRPATH ...]"
   echo ""
   echo "Where:"
   echo " PROGRAM   is the name of the program to test"
   echo " REGRPATH  is 0 or more directories to search for tests (.rt files)"
   exit 0
}

exit_ ()
{
   rm -f $RM
   exit $1
}

msg ()
{
   echo -ne "$*"
   echo -ne "$*" >> $LOGG
}

syntax ()
{
   echo "$1:$2: $3"
}

time_ms() {
   echo "$(($1 / (1000 * 1000)))"
}

time_s() {
   python -c "print '%.3f' % (float ($1) / (1000 * 1000 * 1000))"
}

run_test_line ()
{
   # variables defined at this point:
   # LINE
   # L
   # TEST
   # OUT
   # ERR
   # THIS_PASSED
   # THIS_SKIPPED

   cat $OUT $ERR | bash -c "set -e; $LINE" > /dev/null 2> /dev/null
   EC=$?
   if test $EC = 0; then
      RES="OK"
      THIS_PASSED=$(($THIS_PASSED + 1))
   else
      RES="FAILS"
   fi
   printf "%s:%-3s [%-6s] $EC $LINE\n" "$TEST" "${L}:" $RES >> "$LOG"
}

run_test ()
{
   # cd to the folder and open the test description file, reset the log file
   cd $(dirname "$TESTPATH")
   TEST=$(basename "$TESTPATH")
   exec 9< "$TEST"
   LOG="${TEST}.log"
   > "$LOG"

   # open a temporary file for the preliminary commands
   exec 8> "$PRE"

   # parse the first line, the test name
   read -u 9 DESCR
   if grep -vq '^#' <<< "$DESCR"; then
      syntax $1 1 "expected '# test name'" >> "$LOG";
      cd - > /dev/null; return 1
   fi
   DESCR=$(sed 's/^# //' <<< "$DESCR")

   # dump to PRE all lines before the 'cmd ' keyword
   L=2
   CMD=
   echo 'set -e' >&8
   while read -u 9 LINE; do
      if grep -q '^cmd ' <<< "$LINE"; then
         CMD=$(sed 's/^cmd\s\+//' <<< "$LINE")
         if test -z "$CMD"; then
            syntax $1 $L "expected a shell command" >> "$LOG";
            cd - > /dev/null; return 1
         fi
         break
      fi
      echo "$LINE" >&8
      L=$(($L + 1))
   done
   if test -z "$CMD"; then
      syntax $1 $L "EOF, but expected 'cmd' keyword'" >> "$LOG";
      return 1
   fi

   # close PRE; cd to the folder and run PRE
   exec 8>&-
   bash < $PRE > $OUT 2> $ERR
   EXITCODE=$?

   # dump the first lines to the log file
   echo -e "\nDESCRIPTION    $DESCR\n" >> "$LOG"
   echo "TEST           $TEST" >> "$LOG"
   echo "PROG           $PROG" >> "$LOG"
   echo "PRE-EXITCODE   $EXITCODE" >> "$LOG"

   # run the program
   BEGIN=`date +%s%N`
   bash -c "set -e; $CMD" >> $OUT 2>> $ERR
   EXITCODE=$?
   END=`date +%s%N`
   WALLTIME=$(time_ms $(($END-$BEGIN)))
   WALLTIME_SEC=$(time_s $(($END-$BEGIN)))
   echo "EXITCODE       $EXITCODE" >> "$LOG"
   echo "WALLTIME       ${WALLTIME}ms ($WALLTIME_SEC s)" >> "$LOG"
   echo "" >> "$LOG"

   # iterate through the remaining lines and test them
   THIS_PASSED=0
   THIS_SKIPPED=0
   COUNT=0
   while read -u 9 LINE; do
      L=$(($L + 1))

      # strip and skip whitespace and comments
      LINE=$(sed 's/^\s\+//; s/\s\+$//' <<< "$LINE")
      if test -z "$LINE"; then continue; fi
      if grep -q '^#' <<< "$LINE"; then continue; fi

      COUNT=$(($COUNT + 1))
      run_test_line
   done

   # copy stdout and stderr to the log file
   echo -e "\n== begin stdout ==" >> "$LOG"
   cat $OUT >> "$LOG"
   echo "== end stdout ==" >> "$LOG"
   echo -e "\n== begin stderr ==" >> "$LOG"
   cat $ERR >> "$LOG"
   echo "== end stderr ==" >> "$LOG"

   # the tests passes only if all lines pass; it is skipped only if all lines
   # are skipped
   if test $THIS_PASSED != $COUNT; then THIS_PASSED=0; fi
   if test $THIS_SKIPPED != $COUNT; then THIS_SKIPPED=0; fi

   # close test description and cd back
   exec 9>&-
   cd - > /dev/null
}

find_and_run ()
{
   # create temporary files for stdout, stderr, and the preliminary commands
   OUT=$(mktemp /tmp/regtest.out.XXXXXX)
   ERR=$(mktemp /tmp/regtest.err.XXXXXX)
   PRE=$(mktemp /tmp/regtest.pre.XXXXXX)
   RM="$RM $PRE $OUT $ERR"
   I=1

   msg "Loading tests... "
   NR=$(find $DIRS |grep '.rt$' | wc -l)
   msg "Done.\n\n"
   msg "Running $NR tests:\n\n"

   for TESTPATH in $(find $DIRS | grep '.rt$')
   do
      printf "%03d [Running] %s" $I "$TESTPATH"
      THIS_PASSED=0
      THIS_SKIPPED=0
      run_test
      echo -ne "\r"
      if test $THIS_PASSED != 0; then
         msg "$(printf "%03d [OK     ] ${TESTPATH}.log" $I)\n"
         PASS=$(($PASS + 1))
      elif test $THIS_SKIPPED != 0; then
         msg "$(printf "%03d [SKIPPED] ${TESTPATH}.log" $I)\n"
         SKIP=$(($SKIP + 1))
      else
         msg "$(printf "%03d [FAILS  ] ${TESTPATH}.log" $I)\n"
         FAIL=$(($FAIL + 1))
      fi
      I=$(($I + 1))
   done

   msg "\nDone.\n\n"
   msg "$FAIL FAIL; $PASS PASS; $SKIP SKIPPED\n"
}

locate_program ()
{
   X="$(type -p "$PROG")"
   if test -z "$X"; then
      msg "$PROGNAME: $PROG: unable to locate it\n"
      exit_ 1
   fi
   PROG="$(readlink -f $X)"
}

export_variables ()
{
   export PROG
   export EXITCODE="0"
   export TEST="/dev/null"
   export TESTPATH="/dev/null"
   export OUT="/dev/null"
   export ERR="/dev/null"
   export WALLTIME=0
   LOG="/dev/null"
}

aha ()
{
   exec 10< test.c
   read -u 10 LINE
   echo $?
   echo "$LINE" xxx $(wc -c <<< "$LINE")
   read -u 10 LINE
   echo $?
   echo "$LINE" xxx $(wc -c <<< "$LINE")
   read -u 10 LINE
   echo $?
   echo "$LINE" xxx $(wc -c <<< "$LINE")
   read -u 10 LINE
   echo $?
   echo "$LINE" xxx $(wc -c <<< "$LINE")
}

main ()
{
   LOGG=regression.log.$(date -R | sed 's/ /_/g; s/[,+]//g; s/:/-/g')
   > $LOGG
   rm -f regression.log
   ln -s $LOGG regression.log

   msg "Logging output to \"$LOGG\".\n"

   locate_program
   export_variables
   find_and_run
   exit_ 0
}

# parse arguments
if test $# -eq 0; then usage; fi
PROG=$1
shift
DIRS="$*"

main
