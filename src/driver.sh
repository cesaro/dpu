#!/bin/sh

set -x
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

   # prepare the input
   clang-$LLVMVERS -emit-llvm -c -o $TMP/orig.bc -- "$INPUT" 
   stopif "clang"
	opt-$LLVMVERS -O3 -mem2reg $TMP/orig.bc -o $TMP/opt.bc
   stopif "opt"
	llvm-link-$LLVMVERS $TMP/opt.bc $RT -o $TMP/input.bc
   stopif "llvm-link"

   # dump .bc files, for debugging purposes
   llvm-dis-$LLVMVERS $TMP/orig.bc -o $TMP/orig.ll
   llvm-dis-$LLVMVERS $TMP/input.bc -o $TMP/input.ll

   # run the backend analyzer
   $BACKEND $TMP/input.bc $ARGS
   exit $?
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
