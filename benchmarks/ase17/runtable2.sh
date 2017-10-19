#!/bin/bash

# ==== BEGIN CONFIGURATION VARIABLES ====

# 1s = 1 second; 2m = 2 minutes; 3h = 3 hours
TIMEOUT=8m

# ==== END CONFIGURATION VARIABLES ====


# select the right installation depending on the machine

if test $(hostname) = polaris; then
   DPU=dpu
   NIDHUGG=mynidhugg
else
   DPU=../../../dist/bin/dpu
   NIDHUGGBIN=`which nidhugg`
   NIDHUGG="${NIDHUGGBIN} --c -sc -extfun-no-race=printf -extfun-no-race=write -extfun-no-race=exit -extfun-no-race=atoi -extfun-no-race=pow"
fi

# utilitary functions to run benchmarks
source runlib.sh

compile_bench ()
{
   # setup for Whole Program LLVM
   CC=wllvm
   LLVMVERS=3.7
   export LLVM_COMPILER=clang
   export LLVM_CC_NAME=clang-$LLVMVERS
   export LLVM_CXX_NAME=clang++-$LLVMVERS
   export LLVM_LINK_NAME=llvm-link-$LLVMVERS
   export LLVM_AR_NAME=llvm-ar-$LLVMVERS

   # package mafft 
   MAFFTPROGS="addsingle dndpre makedirectionlist mccaskillwrap pairlocalalign"
   P="$R/debian/mafft-7.123/core/"
   make -C $P $MAFFTPROGS CC=$CC
   for F in $MAFFTPROGS; do
      BC="$F.bc"
      extract-bc "$P/$F" -o "$BC"
   done

   # package blktrace
   P="$R/debian/blktrace-1.0.5/blkiomon"
   make -C $P blkiomon CC=$CC
   extract-bc $P/blkiomon -o blkiomon.bc
}

runall_dpu ()
{
   # pre-conditions:
   # $R       - root of the ase17 folder
   # $TIMEOUT - a timeout specification valid for timeout(1)
   # $DPU     - path to the dpu tool to run

   OPTS='-k0 --mem 350M --stack 2M -O2'
   cp $R/debian/mafft-7.123/test-data/*fasta .
   cp $R/debian/mafft-7.123/test-data/hat2* .
   cp $R/debian/mafft-7.123/test-data/hat3* .
   cp $R/debian/mafft-7.123/test-data/mxscarnamod .
   cp $R/debian/blktrace-1.0.5/test-data/input*dat .
   ls -lh .

   # mafft - addsingle
   N=addsingle
   for n in $(seq 2 2 10); do
      for s in 3; do
         cp hat2.${s}seq hat2
         cp hat3.${s}seq hat3
         CMD="$DPU $N.bc $OPTS -- $N -C $n -K -i ${s}seq.aln.fasta"
         LOG=${N}_threads${n}_seq${s}.log
         run_dpu
      done
   done

   # mafft - dndpre
   N=dndpre
   for n in 2 3 4 5 6 7; do
      for s in 2 4; do
         CMD="$DPU $N.bc $OPTS -- $N -C $n -i ${s}seq.fasta"
         LOG=${N}_threads${n}_seq${s}.log
         run_dpu
      done
   done

   # mafft - makedirectionlist
   N=makedirectionlist
   for n in $(seq 1 5); do
      for s in $(seq 2 4); do
         CMD="$DPU $N.bc $OPTS -- $N -m -I 0 -t 0.01 -C $n -i ${s}seq.fasta"
         LOG=${N}_threads${n}_seq${s}.log
         run_dpu
      done
   done

   # mafft - mccaskillwrap
   N=mccaskillwrap
   for n in 1 2 3; do
      for s in 2; do
         CMD="$DPU $N.bc $OPTS -- $N -C $n -i ${s}seq.fasta -d $PWD"
         LOG=${N}_threads${n}_seq${s}.log
         run_dpu
      done
   done

   # mafft - pairlocalalign
   N=pairlocalalign
   for n in $(seq 1 6); do
      for s in 2 3 4 5; do
         LOG=${N}_threads${n}_seq${s}.log
         CMD="$DPU $N.bc $OPTS -- $N -C $n -i ${s}seq.fasta"
         run_dpu
      done
   done

   # blktrace - blkiomon
   N=blkiomon
   for x in 5 10 15 18 20 22 24 26 28 30; do
      for y in 4 5 6; do
         LOG=${N}_x${x}_y${y}_input1.log
         CMD="$DPU $N.bc $OPTS -- $N -I 1 -x $x -y $y -i input1.dat"
         # -h -
         run_dpu
      done
   done

   # Already include in Table 1:
   ## computing pi
   #preprocess_family $R/pi/pth_pi_mutex.c pi "threads" "1 2 3 4 5 6" "iters" "`seq -w 1000 2000 9000`"
   #for i in pi-threads*.i; do
   #   N=`echo "$i" | sed s/.i$//`
   #   LOG=${N}.log
   #   CMD="$DPU $i $OPTS -- $N"
   #   run_dpu
   #done
}

dump_latex ()
{
   # in a loop, scan all .i files in $R
   # for each .i determine the lowest K such that DPU was optimal
   # using that file determine the number of configurations and events
   # the DPU columns will be from that file
   # the nidhugg columns will be from the _nidhug.log file

   # how to format data with fixed-size columns:
   #ROW=$(printf '%6s & %6s & %6s' $WALLTIME $SSBS $EVENTS)

   echo TODO
}

usage ()
{
   echo "Usage:"
   echo " run.sh        Generates the benchmarks and runs dpu and nidhugg"
   echo " run.sh gen    Generates the bemchmarks (see folder $R)"
   echo " run.sh run    Assumes that BLA "
}

test_can_run ()
{
   echo
   echo "$DPU --help"
   $DPU --help
   $DPU --version

   echo
   echo "$NIDHUGG --help"
   $NIDHUGG --help
   $NIDHUGG --version

   echo
   echo If you see error messages above this line,
   echo then check that you understand what you are doing.
   echo
}

main ()
{
   print_date "Starting the script"
   test_can_run
   compile_bench
   print_date "Running tool DPU"
   runall_dpu
   print_date "Generating latex tables"
   dump_latex
   print_date "Finished"
}


R=table2.$(date -R | sed -e 's/ /_/g' -e 's/[,+]//g' -e 's/:/-/g')
rm -Rf $R latest.table2
mkdir $R
ln -s $R latest.table2
cd $R

R=..
main 2>&1 | tee XXX.log

