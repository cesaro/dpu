#!/bin/sh

# settings
TMP=/tmp/dpu.$USER #.$$
LLVMVERS=3.7
PREFIX=$(readlink -f $(dirname "$0"))/..
RT=$PREFIX/lib/dpu/rt.bc
BACKEND=$PREFIX/lib/dpu/dpu-backend

# input parameters
PROGNAME=$0
INPUT=
ARGS=
GDB=0

usage ()
{
   $BACKEND --help
   exit 1
}

stopif ()
{
   STAT=$?
   if test $STAT -ne 0
   then
      echo "$PROGNAME: $1 exited with status $STAT"
      exit $STAT
   fi
}

main_ ()
{
   # preliminary actions
   rm -Rf $TMP
   mkdir $TMP
   if test ! -e "$INPUT"
   then
      echo "dpu: $INPUT: File not found" >&2
      exit 1
   fi

set -x
   # prepare the input
   if echo "$INPUT" | grep -q '\.c$\|\.i$'; then
      clang-$LLVMVERS -O3 -emit-llvm -c -o $TMP/opt.bc -- "$INPUT" 
      stopif "clang"
   else
      opt-$LLVMVERS -mem2reg "$INPUT" -o $TMP/opt.bc
      stopif "opt"
   fi
   llvm-link-$LLVMVERS $TMP/opt.bc $RT -o $TMP/input.bc
   stopif "llvm-link"

   # dump .bc files, for debugging purposes
   #llvm-dis-$LLVMVERS $TMP/orig.bc -o $TMP/orig.ll
   #llvm-dis-$LLVMVERS $TMP/input.bc -o $TMP/input.ll

   # run the backend analyzer
   #echo "$BACKEND $TMP/input.bc $ARGS"
   if test $GDB = 1; then
      gdb $BACKEND \
         -ex 'break breakme' \
         -ex 'info break' \
         -ex "run $TMP/input.bc $ARGS"
   elif test $GDB = 2; then
      gdb $BACKEND \
         -ex "run $TMP/input.bc $ARGS"
   elif test $GDB = 3; then
      valgrind $BACKEND $TMP/input.bc $ARGS
   else
      $BACKEND $TMP/input.bc $ARGS
      exit $?
   fi

   # FIXME - remote this from ehre, build svgs if we detect dot files in tmp
   #for f in /tmp/dot/*.dot; do dot -Tsvg -O $f; done
}

# parse arguments and call main_
if test $# -eq 0;
then
   usage
fi
INPUT=$1
first=1
ARGS=
for a in $@;
do
   if test $first -eq 1; then first=0; continue; fi
   if test $a = '--gdb'; then GDB=1; continue; fi
   if test $a = '--gdb2'; then GDB=2; continue; fi
   if test $a = '--valgrind'; then GDB=3; continue; fi
   ARGS="$ARGS $a"
done
if test "$INPUT" = "--help"; then
   $BACKEND --help
   exit 0
fi
if test "$INPUT" = "-h"; then
   $BACKEND --help
   exit 0
fi
if test "$INPUT" = "-V"; then
   $BACKEND --version
   exit 0
fi
if test "$INPUT" = "--version"; then
   $BACKEND --version
   exit 0
fi
main_
