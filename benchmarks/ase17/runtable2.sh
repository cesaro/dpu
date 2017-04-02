#!/bin/bash

# ==== BEGIN CONFIGURATION VARIABLES ====

# 1s = 1 second; 2m = 2 minutes; 3h = 3 hours
TIMEOUT=20m

# ==== END CONFIGURATION VARIABLES ====


# select the right installation depending on the machine

if test $(hostname) = mariapacum; then
   DPU=../../../dist/bin/dpu
   NIDHUGG="/usr/local/bin/nidhuggc --c -sc --nidhugg=/usr/local/bin/nidhugg -extfun-no-race=printf -extfun-no-race=write -extfun-no-race=exit -extfun-no-race=atoi" 
elif test $(hostname) = polaris; then
   DPU=dpu
   NIDHUGG=mynidhugg
elif test $(hostname) = poet; then
   DPU="/home/msousa/dpu2/dist/bin/dpu"
   NIDHUGG="nidhuggc --c -sc -extfun-no-race=printf -extfun-no-race=write -extfun-no-race=exit -extfun-no-race=atoi"
elif test $(hostname) = polaris; then
   DPU=dpu
   NIDHUGG=mynidhugg
else
   DPU=../../../dist/bin/dpu
   NIDHUGGBIN=`which nidhugg`
   NIDHUGG="${NIDHUGGBIN} --c -sc -extfun-no-race=printf -extfun-no-race=write -extfun-no-race=exit -extfun-no-race=atoi" 
fi

# utilitary functions to run benchmarks
source runlib.sh

compile_bench ()
{
   CC=wllvm

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

run_dpu ()
{
   # expects the following variables to be defiend:
   # $N       - identifier of the benchmark
   # $CMD     - the command to run
   # $LOG     - the path to the log file to generate
   # $TIMEOUT - a timeout specification valid for timeout(1)

   CMD="time timeout $TIMEOUT $CMD"
   echo "name      $N" > $LOG
   echo "cmd       $CMD" >> $LOG

   # run the program
   echo "$CMD"
   echo "> $LOG"
   BEGIN=`date +%s%N`
   $CMD > ${LOG}.stdout 2> ${LOG}.stderr
   EXITCODE=$?
   END=`date +%s%N`

   # check for timeouts
   if test "$EXITCODE" = 124; then
      DEFECTS="-"
      WALLTIME="TO"
      MAXCONFS="-"
      SSBS="-"
      EVENTS="-"
   elif test "$EXITCODE" != 0; then
      if grep "dpu: error.*unhandled" ${LOG}.{stdout,stderr}; then
         WALLTIME="MO"
      else
         WALLTIME="ERR"
      fi
      DEFECTS="-"
      MAXCONFS="-"
      SSBS="-"
      EVENTS="-"
   else
      WALLTIME=`round $(($END-$BEGIN))`
      DEFECTS=$(grep "dpu: summary" ${LOG}.stdout | awk '{print $3}')
      MAXCONFS=$(grep "dpu: summary" ${LOG}.stdout | awk '{print $5}')
      SSBS=$(grep "dpu: summary" ${LOG}.stdout | awk '{print $7}')
      EVENTS=$(grep "dpu: summary" ${LOG}.stdout | awk '{print $9}')
   fi

   # dump numbers to the log file
   echo "" >> $LOG
   echo "exitcode  $EXITCODE" >> $LOG
   echo "begin     $BEGIN" >> $LOG
   echo "end       $END" >> $LOG
   echo "defects   $DEFECTS" >> $LOG
   echo "walltime  $WALLTIME" >> $LOG
   echo "maxconfs  $MAXCONFS" >> $LOG
   echo "SSBs      $SSBS" >> $LOG
   echo "events    $EVENTS" >> $LOG

   # print a summary
   printf 'WTIME=%-8s MAXCONF=%-4s SSBS=%-5s EVENTS=%-5s DEFECTS=%-2s\n\n' $WALLTIME $MAXCONFS $SSBS $EVENTS $DEFECTS

   echo -e "\n\nstdout:" >> $LOG
   cat ${LOG}.stdout >> $LOG
   echo -e "\nstderr:" >> $LOG
   cat ${LOG}.stderr >> $LOG
   rm ${LOG}.stdout
   rm ${LOG}.stderr
}

runall_dpu ()
{
   # pre-conditions:
   # $R       - root of the ase17 folder
   # $TIMEOUT - a timeout specification valid for timeout(1)
   # $DPU     - path to the dpu tool to run

   OPTS='-a0 -v --mem 350M --stack 2M -O1'
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
         CMD="$DPU $N.bc $OPTS -- $N -I 1 -x $x -y $y -h - -i input1.dat"
         run_dpu
      done
   done
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
   echo If you see errors above this line,
   echo then check that you understand what you are doing.
   echo
}

main ()
{
   test_can_run
   compile_bench
   runall_dpu
   dump_latex
}


R=table2.$(date -R | sed -e 's/ /_/g' -e 's/[,+]//g' -e 's/:/-/g')
rm -Rf $R latest.table2
mkdir $R
ln -s $R latest.table2
cd $R

R=..
main 2>&1 | tee XXX.log

