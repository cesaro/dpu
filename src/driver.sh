#!/bin/sh

# settings
LLVMVERS=6.0
PREFIX=$(readlink -f $(dirname $(readlink -f "$0")))/..
RT=$PREFIX/lib/dpu/rt.bc
INC=$PREFIX/include
BACKEND=$PREFIX/lib/dpu/dpu-backend

# input parameters
PROGNAME=$0
INPUT=
ARGS=
DEFS=
GDB=0
TMP=/tmp/will-change

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

cleanup ()
{
   # remove temporary files
   rm -f $TMP*
}

main_ ()
{
   if test ! -e "$INPUT"
   then
      echo "dpu: $INPUT: File not found" >&2
      exit 1
   fi

   # prepare the input
   if echo "$INPUT" | grep -q '.\.c$'; then
      CMD="clang-$LLVMVERS -I $INC -D__DPU__ $DEFS -O3 -emit-llvm -c -o ${TMP}.opt.bc -- '$INPUT'"
      echo $CMD
      eval $CMD
      stopif "clang"
   elif echo "$INPUT" | grep -q '.\.i$'; then
      CMD="clang-$LLVMVERS -D__DPU__ $DEFS -O3 -emit-llvm -c -o ${TMP}.opt.bc -- '$INPUT'"
      echo $CMD
      eval $CMD
      stopif "clang"
   else
      CMD="opt-$LLVMVERS -mem2reg '$INPUT' -o ${TMP}.opt.bc"
      echo $CMD
      eval $CMD
      stopif "opt"
   fi
   CMD="llvm-link-$LLVMVERS ${TMP}.opt.bc $RT -o ${TMP}.bc"
   echo $CMD
   eval $CMD
   stopif "llvm-link"

   # dump .bc files, for debugging purposes
   #llvm-dis-$LLVMVERS ${TMP}.opt.bc -o ${TMP}.opt.ll
   #llvm-dis-$LLVMVERS ${TMP}.bc -o ${TMP}.ll

   # run the backend analyzer
   if test $GDB = 1; then
      CMD="gdb $BACKEND -ex 'break breakme' -ex 'info break' -ex \"run ${TMP}.bc $ARGS\""
   elif test $GDB = 2; then
      CMD="gdb $BACKEND -ex \"run ${TMP}.bc $ARGS\""
   elif test $GDB = 3; then
      CMD="cgdb $BACKEND -ex \"run ${TMP}.bc $ARGS\""
   elif test $GDB = 4; then
      CMD="valgrind --tool=callgrind --dump-instr=yes $BACKEND ${TMP}.bc $ARGS"
   else
      CMD="$BACKEND ${TMP}.bc $ARGS"
   fi

   echo $CMD
   eval $CMD
   exit $?
}

# parse arguments and call main_
if test $# -eq 0;
then
   usage
fi

while test $# -ge 1; do
   case $1 in
   -h | --help)
      $BACKEND -h
      exit 0
      ;;
   -V | --ver | --vers | --versi | --versio | --version)
      $BACKEND -V
      exit 0
      ;;
   --gdb)
      GDB=1
      ;;
   --gdb2)
      GDB=2
      ;;
   --gdb3)
      GDB=3
      ;;
   --callgrind | --ca | --call | --callg | --callgr | --callgri | --callgrin)
      GDB=4
      ;;
   -D)
      shift
      DEFS="$DEFS -D$1"
      ;;
   -D*)
      #DEFS="$DEFS -D $(cut -c 3- <<< '$1')"
      DEFS="$DEFS -D$(echo "$1" | cut -c 3-)"
      ;;
   --)
      shift
      if test -z "$INPUT"; then
         INPUT=$1
         shift
      fi
      ARGS="$ARGS -- $*"
      break
      ;;
   *)
      if test -z "$INPUT"; then
         INPUT=$1
      else
         ARGS="$ARGS $1"
      fi
      ;;
   esac
   shift
done

TMP=$(mktemp -t dpu.XXXX.$(basename -- "$INPUT"))
trap cleanup EXIT
trap cleanup INT
trap cleanup QUIT
trap cleanup TERM

main_
