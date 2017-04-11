#!/bin/bash

# ==== BEGIN CONFIGURATION VARIABLES ====

# 1s = 1 second; 2m = 2 minutes; 3h = 3 hours
TIMEOUT=15m

# ==== END CONFIGURATION VARIABLES ====


# select the right installation depending on the machine

if test $(hostname) = mariapacum; then
   DPU=../../dist/bin/dpu
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
   DPU=../../dist/bin/dpu
   NIDHUGGBIN=`which nidhugg`
   NIDHUGG="${NIDHUGGBIN} --c -sc -extfun-no-race=printf -extfun-no-race=write -extfun-no-race=exit -extfun-no-race=atoi" 
fi

# utilitary functions to run benchmarks
source runlib.sh

generate_bench ()
{
   # pre-conditions:
   # $R       - root of the ase17 folder

   preprocess_family $R/dispatcher.c dispatch   "serv" "1 2 3 4 5 6 8 10 12 14" "reqs" "1 2 3 4"

   preprocess_family $R/mpat.c       mpat       "k" "`seq -w 1 8`"

   preprocess_family $R/poke.c       poke       "threads" "2 3 4" "iters" "2 3 4 5 6"
   preprocess_family $R/poke.c       poke       "threads" "5 6 7 8" "iters" "1 2 3 4"

   preprocess_family $R/multiprodcon.c multipc  "workers" "3 4 5 6 7" "prods" "1 2 3 4 5"

   preprocess_family $R/pi/pth_pi_mutex.c pi    "threads" "1 2 3 4 5 6" "iters" "`seq -w 1000 2000 9000`"

   # these are deemed to be not realistic:
   #preprocess_family $R/spat.c       spat         "threads" "1 2 3 4 5 6" "mut" "1 2 3 4 5"
   #preprocess_family $R/ssb3.c       ssb3         "writers" "`seq -w 1 9`" "seqlen" "2 4 6 8"
   #preprocess_family $R/ssbexp.c     ssbexp       "writers" "`seq -w 1 18`"
}

generate_bench_smallest ()
{
   # pre-conditions:
   # $R       - root of the ase17 folder

   preprocess_family $R/dispatcher.c dispatch   "serv" "3 4" "reqs" "2 3"
   preprocess_family $R/mpat.c       mpat       "k" "`seq -w 2 6`"
   preprocess_family $R/poke.c       poke       "th" "1 2 3" "iters" "`seq -w 1 2 6`"
   preprocess_family $R/multiprodcon.c multipc  "workers" "3 4 5" "prods" "2 3"
   preprocess_family $R/pi/pth_pi_mutex.c pi    "threads" "`seq -w 1 3`" "iters" "`seq -w 1000 2000 9000`"

   preprocess_family $R/spat.c       spat       "threads" "2 3 4" "mut" "2 3"
   preprocess_family $R/ssb3.c       ssb3       "writers" "`seq -w 2 5`" "seqlen" "4 6"
   preprocess_family $R/ssbexp.c     ssbexp     "writers" "`seq -w 2 5`"
}

generate_bench_cesar ()
{
   # pre-conditions:
   # $R       - root of the ase17 folder

   #preprocess_family $R/dispatcher.c dispatch   "serv" "3" "reqs" "4"
   #preprocess_family $R/mpat.c       mpat       "k" "2"
   #preprocess_family $R/spat.c       spat       "threads" "2 3" "mut" "2"

   #preprocess_family $R/dispatcher.c dispatcher   "serv" "1 2 3 4 5 6 8" "reqs" "2 3"
   #preprocess_family $R/mpat.c       mpat         "k" "4 5 6 7 8"

   #preprocess_family $R/poke.c       poke         "threads" "2 3 4" "iters" "2 3 4 5 6"
   #preprocess_family $R/poke.c       poke         "threads" "5 6 7 8" "iters" "1 2 3 4"

   #preprocess_family $R/poke.c       poke         "threads" "4" "iters" "4 6"
   #preprocess_family $R/poke.c       poke         "threads" "5" "iters" "4"
   #preprocess_family $R/poke.c       poke         "threads" "6" "iters" "4"
   #preprocess_family $R/poke.c       poke         "threads" "7" "iters" "4"
   #preprocess_family $R/poke.c       poke         "threads" "8" "iters" "4"

   #preprocess_family $R/multiprodcon.c multipc     "workers" "3 4 5 6 7" "prods" "1 2 3 4 5"

   #preprocess_family $R/ssb3.c       ssb3         "writers" "`seq -w 1 9`" "seqlen" "2 4 6 8"
   #preprocess_family $R/ssbexp.c     ssbexp       "writers" "`seq -w 1 18`"
   #preprocess_family $R/pi/pth_pi_mutex.c pi      "threads" "`seq -w 1 6`" "iters" "`seq -w 1000 2000 9000`"
}


runall_dpu ()
{
   # pre-conditions:
   # $TIMEOUT - a timeout specification valid for timeout(1)
   # $DPU     - path to the dpu tool to run

   OPTS="--mem 128M --stack 6M -O2"
   for i in *.i; do
      N=`echo "$i" | sed s/.i$//`

      ## -a-1
      #LOG=${N}_dpu_alt-1.log
      #CMD="$DPU $i -a-1 $OPTS"
      #run_dpu

      # -a0
      LOG=${N}_dpu_alt0.log
      CMD="$DPU $i -a0 $OPTS"
      run_dpu

      # k-partial
      for a in 1 2 3 4; do
         LOG=${N}_dpu_alt${a}.log
         CMD="$DPU $i -a$a $OPTS"
         run_dpu

         # if we got 0 SSBs we skip higher -a
         if test "$SSBS" = 0; then break; fi
      done
   done
}

runall_nidhugg ()
{
   # pre-conditions:
   # $TIMEOUT - a timeout specification valid for timeout(1)
   # $DPU     - path to the dpu tool to run
   for i in *.i; do
      N=`echo "$i" | sed s/.i$//`
      LOG=${N}_nidhugg.log
      CMD="$NIDHUGG $i"
      run_nidhugg
   done
}

dump_latex ()
{
   echo "Generating latex table ..."

   echo "% Benchmark              DPU (k=1)      DUP (k=2)      DUP (k=3)      DUP (optimal)  Nidhugg" >> table.tex
   echo "% Name  LOCs Thrs Confs  Time Mem SSBs  Time Mem SSBs  Time Mem SSBs  Time Mem SSBs  Time Mem SSBs" >> table.tex

   INSTANCES=$(ls *.i | sed 's/.i$//' | sort -u)

   for i in $INSTANCES; do
      echo "$i"
      # check that all tools agree on the number of configs
      MAXCONFS=$(cat ${i}_dpu* ${i}_nidhugg* | grep '^maxconfs ' | sort -u | grep -v 'maxconfs *-$')
      NUM=$(wc -l <<< "$MAXCONFS")
      if test "$NUM" -gt 1; then
         echo "WARNING: $i: tools report != number of configurations"
      fi
      MAXCONFS=$(head -n1 <<< "$MAXCONFS" | awk '{print $2}')

      # check that all -aX runs of DPU agree on the number threads
      NUMTHREADS=$(cat ${i}_dpu* | grep '^dpu: stats: unfolding: .* threads created$' | sort -u)
      NUM=$(wc -l <<< "$NUMTHREADS")
      if test "$NUM" -gt 1; then
         echo "WARNING: $i: dpu reports != number of threads on different runs"
      fi
      NUMTHREADS=$(head -n1 <<< "$NUMTHREADS" | awk '{print $4}')

      # name, loc, numthreads, maxconfs
      ROW=$(printf '%-25s &  LOC & %8s & %8s' $i $NUMTHREADS $MAXCONFS)

      # columns for DPU
      for a in 1 2 3 0; do
         WTIME=$(cat ${i}_dpu_alt${a}.log 2> /dev/null | grep '^walltime ' | sed 's/^walltime //')
         MAXRSS=$(cat ${i}_dpu_alt${a}.log 2> /dev/null | grep '^maxrss ' | sed 's/^maxrss //')
         SSBS=$(cat ${i}_dpu_alt${a}.log 2> /dev/null | grep '^SSBs ' | sed 's/^SSBs //')
         # time, memory, ssbs
         ROW="$ROW$(printf ' & %8s & %8s & %8s' $WTIME $MAXRSS $SSBS)"
      done

      # columns for NIDHUGG
      WTIME=$(cat ${i}_nidhugg.log 2> /dev/null | grep '^walltime ' | sed 's/^walltime //')
      MAXRSS=$(cat ${i}_nidhugg.log 2> /dev/null | grep '^maxrss ' | sed 's/^maxrss //')
      SSBS=$(cat ${i}_nidhugg.log 2> /dev/null | grep '^SSBs ' | sed 's/^SSBs //')
      ROW="$ROW$(printf '  & %8s & %8s & %8s' $WTIME $MAXRSS $SSBS)"

      echo "$ROW" '\newrow' >> table.tex
   done

   echo "done!"
}

test_can_run ()
{
   echo
   echo "$DPU --help"
   $DPU --help
   echo "$DPU --version"
   $DPU --version

   echo
   echo "$NIDHUGG --help"
   $NIDHUGG --help
   echo "$NIDHUGG --version"
   $NIDHUGG --version

   echo
   echo If you see error messages above this line,
   echo then check that you understand what you are doing.
   echo
}

main ()
{
   test_can_run
   generate_bench
   #generate_bench_smallest
   #generate_bench_cesar
   runall_dpu
   runall_nidhugg
   dump_latex
}


R=table1.$(date -R | sed -e 's/ /_/g' -e 's/[,+]//g' -e 's/:/-/g')
rm -Rf $R latest.table1
mkdir $R
ln -s $R latest.table1
cd $R

R=..
main 2>&1 | tee XXX.log

