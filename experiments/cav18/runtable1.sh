#!/bin/bash

# ==== BEGIN CONFIGURATION VARIABLES ====

# 1s = 1 second; 2m = 2 minutes; 3h = 3 hours
TIMEOUT=8m

WANT_DPU_ALT_SDPOR=n
WANT_DPU_ALT0=y
WANT_DPU_ALT1=y
WANT_DPU_ALT2=y
WANT_DPU_ALT3=y
WANT_DPU_ALT4=n
WANT_NIDHUGG=y

DPU_OPTS="-O1"

# ==== END CONFIGURATION VARIABLES ====

# utilitary functions to run benchmarks
source runlib.sh

generate_bench_all ()
{
   # pre-conditions:
   # $R       - root of the cav18 folder

   preprocess_family $R/bench/dispatcher.c dispatch   "serv" "1 2 3 4 5 6 8 10 12 14" "reqs" "1 2 3 4"
   preprocess_family $R/bench/mpat.c       mpat       "k" "`seq -w 1 8`"
   preprocess_family $R/bench/poke.c       poke       "threads" "2 3 4" "iters" "2 3 4 5 6"
   preprocess_family $R/bench/poke.c       poke       "threads" "5 6 7 8" "iters" "1 2 3 4"
   preprocess_family $R/bench/multiprodcon.c multipc  "prods" "1 2 3 4 5" "workers" "3 4 5 6 7"
   preprocess_family $R/bench/pi/pth_pi_mutex.c pi    "threads" "1 2 3 4 5 6" "iters" "`seq -w 1000 2000 9000`"

   # these are deemed to be not realistic:
   preprocess_family $R/bench/spat.c       spat         "threads" "1 2 3 4 5 6" "mut" "1 2 3 4 5"
   preprocess_family $R/bench/ssb3.c       ssb3         "writers" "`seq -w 1 9`" "seqlen" "2 4 6 8"
   preprocess_family $R/bench/ssbexp.c     ssbexp       "writers" "`seq -w 1 18`"
}

generate_bench_selection ()
{
   # The benchmarks selected for Table 1 of the paper

   # pre-conditions:
   # $R       - root of the cav18 folder

   preprocess_family $R/bench/dispatcher.c dispatch   "serv" "5" "reqs" "`seq -w 2 6`"
   preprocess_family $R/bench/mpat.c       mpat       "k" "`seq -w 4 8`"
   preprocess_family $R/bench/multiprodcon.c multipc  "prods" "2 3 4 5" "workers" "5"
   preprocess_family $R/bench/pi/pth_pi_mutex.c pi    "threads" "`seq -w 5 8`" "iters" "5000"
   preprocess_family $R/bench/poke.c       poke       "threads" "`seq -w 7 12`" "iters" "3"
}

generate_bench_selection_below10s ()
{
   # The same as in generate_bench_selection but restricted to those yielding a
   # running time below 20 secs

   # pre-conditions:
   # $R       - root of the cav18 folder

   preprocess_family $R/bench/dispatcher.c dispatch   "serv" "5" "reqs" "`seq -w 2 4`"
   preprocess_family $R/bench/mpat.c       mpat       "k" "`seq -w 4 5`"
   preprocess_family $R/bench/multiprodcon.c multipc  "prods" "2 3" "workers" "5"
   preprocess_family $R/bench/pi/pth_pi_mutex.c pi    "threads" "`seq -w 5 7`" "iters" "5000"
   preprocess_family $R/bench/poke.c       poke       "threads" "`seq -w 7 12`" "iters" "3"
}



generate_bench_smallest ()
{
   # pre-conditions:
   # $R       - root of the cav18 folder

   preprocess_family $R/bench/dispatcher.c dispatch   "serv" "3 4" "reqs" "2 3"
   preprocess_family $R/bench/mpat.c       mpat       "k" "`seq -w 2 6`"
   preprocess_family $R/bench/poke.c       poke       "th" "1 2 3" "iters" "`seq -w 1 2 6`"
   preprocess_family $R/bench/multiprodcon.c multipc  "workers" "3 4 5" "prods" "2 3"
   preprocess_family $R/bench/pi/pth_pi_mutex.c pi    "threads" "`seq -w 1 3`" "iters" "`seq -w 1000 2000 9000`"

   preprocess_family $R/bench/spat.c       spat       "threads" "2 3 4" "mut" "2 3"
   preprocess_family $R/bench/ssb3.c       ssb3       "writers" "`seq -w 2 5`" "seqlen" "4 6"
   preprocess_family $R/bench/ssbexp.c     ssbexp     "writers" "`seq -w 2 5`"
}

generate_bench_smallruntime ()
{
   # a selection of benchmarks with in the range 60 to 46K max confs and 0.4
   # secs - 63 secs running time, representative of all programs in the
   # benchmark:

   #LOG,                                     WTIME,   MAXRSS,  MAXCON,  SSBS,    EVENTS,  DEFECTS,
   #dispatch-serv1_reqs4_dpu_alt0.log,       0.476,   37,      80,      0,       1228,    0,
   #dispatch-serv2_reqs3_dpu_alt0.log,       0.488,   37,      150,     0,       2057,    0,
   #dispatch-serv2_reqs4_dpu_alt0.log,       0.613,   38,      648,     0,       9239,    0,
   #dispatch-serv3_reqs3_dpu_alt0.log,       0.552,   37,      398,     0,       5758,    0,
   #dispatch-serv3_reqs4_dpu_alt0.log,       1.358,   37,      2504,    0,       37166,   0,
   #dispatch-serv4_reqs4_dpu_alt0.log,       3.523,   59,      6854,    0,       108925,  0,
   #dispatch-serv5_reqs4_dpu_alt0.log,       8.289,   118,     15282,   0,       261740,  0,
   #dispatch-serv6_reqs3_dpu_alt0.log,       1.477,   37,      2414,    0,       43993,   0,
   #dispatch-serv6_reqs4_dpu_alt0.log,       24.711,  238,     29756,   0,       549107,  0,
   #dispatch-serv8_reqs3_dpu_alt0.log,       3.150,   65,      5298,    0,       111473,  0,
   #dispatch-serv10_reqs3_dpu_alt0.log,      7.486,   122,     9862,    0,       236009,  0,
   #dispatch-serv12_reqs3_dpu_alt0.log,      16.844,  223,     16490,   0,       442897,  0,
   #dispatch-serv14_reqs2_dpu_alt0.log,      0.953,   37,      884,     0,       26255,   0,
   #mpat-k4_dpu_alt0.log,                    0.556,   37,      384,     0,       3822,    0,
   #mpat-k5_dpu_alt0.log,                    2.747,   37,      3840,    0,       38017,   0,
   #mpat-k6_dpu_alt0.log,                    63.426,  218,     46080,   0,       455876,  0,
   #multipc-workers3_prods2_dpu_alt0.log,    0.441,   38,      66,      0,       499,     0,
   #multipc-workers3_prods3_dpu_alt0.log,    1.272,   38,      2430,    0,       4190,    0,
   #multipc-workers4_prods2_dpu_alt0.log,    0.448,   38,      54,      0,       274,     0,
   #multipc-workers4_prods3_dpu_alt0.log,    1.844,   38,      3486,    0,       14833,   0,
   #multipc-workers5_prods3_dpu_alt0.log,    1.718,   38,      2958,    0,       7832,    0,
   #multipc-workers7_prods3_dpu_alt0.log,    1.793,   38,      2763,    0,       6364,    0,
   #pi-threads6_iters1000_dpu_alt0.log,      0.714,   38,      720,     0,       10221,   0,
   #poke-threads3_iters4_dpu_alt0.log,       0.784,   37,      711,     0,       12630,   0,
   #poke-threads3_iters5_dpu_alt0.log,       2.085,   38,      2970,    0,       54543,   0,
   #poke-threads3_iters6_dpu_alt0.log,       8.114,   101,     11853,   0,       223536,  0,
   #poke-threads4_iters5_dpu_alt0.log,       9.851,   129,     15936,   0,       294107,  0,
   #poke-threads5_iters3_dpu_alt0.log,       0.815,   37,      844,     0,       15655,   0,
   #poke-threads5_iters4_dpu_alt0.log,       5.093,   71,      7079,    0,       133846,  0,
   #poke-threads6_iters4_dpu_alt0.log,       11.906,  142,     15624,   0,       313095,  0,
   #poke-threads7_iters3_dpu_alt0.log,       1.881,   39,      2440,    0,       51317,   0,
   #poke-threads7_iters4_dpu_alt0.log,       32.278,  286,     30239,   0,       643982,  0,
   #poke-threads8_iters3_dpu_alt0.log,       2.687,   52,      3700,    0,       82819,   0,

   preprocess_family $R/bench/dispatcher.c dispatch   "serv" "1"       "reqs" "4"
   preprocess_family $R/bench/dispatcher.c dispatch   "serv" "2 3 6"   "reqs" "3 4"
   preprocess_family $R/bench/dispatcher.c dispatch   "serv" "4 5"     "reqs" "4"
   preprocess_family $R/bench/dispatcher.c dispatch   "serv" "8 10 12" "reqs" "3"
   preprocess_family $R/bench/dispatcher.c dispatch   "serv" "14"      "reqs" "2"

   preprocess_family $R/bench/mpat.c       mpat       "k" "4 5 6"

   preprocess_family $R/bench/multiprodcon.c multipc  "workers" "3 4"  "prods" "2 3"
   preprocess_family $R/bench/multiprodcon.c multipc  "workers" "5 7"  "prods" "3"

   preprocess_family $R/bench/pi/pth_pi_mutex.c pi    "threads" "6"    "iters" "1000"

   preprocess_family $R/bench/poke.c       poke       "th" "3"     "iters" "4 5 6"
   preprocess_family $R/bench/poke.c       poke       "th" "4"     "iters" "5"
   preprocess_family $R/bench/poke.c       poke       "th" "5 6 7" "iters" "4"
   preprocess_family $R/bench/poke.c       poke       "th" "5 7 8" "iters" "3"
}

generate_bench_morethan1sec()
{
   # LOG,                                     WTIME,   MAXRSS,  MAXCON,  SSBS,    EVENTS,  DEFECTS,
   # poke-threads6_iters3_dpu_alt0.log,       1.214,   37,      1504,    0,       29687,   0,      
   # multipc-workers3_prods3_dpu_alt0.log,    1.272,   38,      2430,    0,       4190,    0,      
   # dispatch-serv3_reqs4_dpu_alt0.log,       1.358,   37,      2504,    0,       37166,   0,      
   # dispatch-serv6_reqs3_dpu_alt0.log,       1.477,   37,      2414,    0,       43993,   0,      
   # multipc-workers6_prods3_dpu_alt0.log,    1.539,   37,      2430,    0,       4211,    0,      
   # multipc-workers5_prods3_dpu_alt0.log,    1.718,   38,      2958,    0,       7832,    0,      
   # multipc-workers7_prods3_dpu_alt0.log,    1.793,   38,      2763,    0,       6364,    0,      
   # multipc-workers4_prods3_dpu_alt0.log,    1.844,   38,      3486,    0,       14833,   0,      
   # poke-threads7_iters3_dpu_alt0.log,       1.881,   39,      2440,    0,       51317,   0,      
   # poke-threads4_iters4_dpu_alt0.log,       1.885,   37,      2636,    0,       47519,   0,      
   # poke-threads3_iters5_dpu_alt0.log,       2.085,   38,      2970,    0,       54543,   0,      
   # poke-threads8_iters3_dpu_alt0.log,       2.687,   52,      3700,    0,       82819,   0,      
   # mpat-k5_dpu_alt0.log,                    2.747,   37,      3840,    0,       38017,   0,      
   # dispatch-serv8_reqs3_dpu_alt0.log,       3.150,   65,      5298,    0,       111473,  0,      
   # dispatch-serv4_reqs4_dpu_alt0.log,       3.523,   59,      6854,    0,       108925,  0,      
   # poke-threads5_iters4_dpu_alt0.log,       5.093,   71,      7079,    0,       133846,  0,      
   # dispatch-serv10_reqs3_dpu_alt0.log,      7.486,   122,     9862,    0,       236009,  0,      
   # poke-threads3_iters6_dpu_alt0.log,       8.114,   101,     11853,   0,       223536,  0,      
   # dispatch-serv5_reqs4_dpu_alt0.log,       8.289,   118,     15282,   0,       261740,  0,      
   # poke-threads4_iters5_dpu_alt0.log,       9.851,   129,     15936,   0,       294107,  0,      
   # poke-threads6_iters4_dpu_alt0.log,       11.906,  142,     15624,   0,       313095,  0,      
   # dispatch-serv12_reqs3_dpu_alt0.log,      16.844,  223,     16490,   0,       442897,  0,      
   # dispatch-serv6_reqs4_dpu_alt0.log,       24.711,  238,     29756,   0,       549107,  0,      
   # poke-threads7_iters4_dpu_alt0.log,       32.278,  286,     30239,   0,       643982,  0,      
   # dispatch-serv14_reqs3_dpu_alt0.log,      36.889,  386,     25566,   0,       762041,  0,      
   # mpat-k6_dpu_alt0.log,                    63.426,  218,     46080,   0,       455876,  0,      
   # poke-threads4_iters6_dpu_alt0.log,       74.390,  677,     92032,   0,       1733147, 0,      
   # poke-threads8_iters4_dpu_alt0.log,       81.268,  528,     53276,   0,       1205335, 0,      
   # multipc-workers4_prods4_dpu_alt0.log,    144.232, 63,      204120,  0,       121526,  0,      
   # dispatch-serv8_reqs4_dpu_alt0.log,       188.098, 797,     86634,   0,       1840121, 0,      
   # multipc-workers6_prods4_dpu_alt0.log,    223.635, 108,     262908,  0,       229072,  0,      
   # multipc-workers3_prods4_dpu_alt0.log,    263.388, 290,     381312,  0,       724955,  0,      
   # multipc-workers5_prods4_dpu_alt0.log,    268.965, 244,     314064,  0,       578835,  0,      
   # multipc-workers7_prods4_dpu_alt0.log,    282.783, 121,     261261,  0,       254267,  0,  


   preprocess_family $R/bench/dispatcher.c      dispatch serv 10  reqs 3
   preprocess_family $R/bench/dispatcher.c      dispatch serv 12  reqs 3
   preprocess_family $R/bench/dispatcher.c      dispatch serv 14  reqs 3
   #preprocess_family $R/bench/dispatcher.c      dispatch serv 3   reqs 4
   preprocess_family $R/bench/dispatcher.c      dispatch serv 4   reqs 4
   preprocess_family $R/bench/dispatcher.c      dispatch serv 5   reqs 4
   #preprocess_family $R/bench/dispatcher.c      dispatch serv 6   reqs 3
   preprocess_family $R/bench/dispatcher.c      dispatch serv 6   reqs 4
   preprocess_family $R/bench/dispatcher.c      dispatch serv 8   reqs 3
   preprocess_family $R/bench/dispatcher.c      dispatch serv 8   reqs 4
   preprocess_family $R/bench/mpat.c      mpat  k 5
   preprocess_family $R/bench/mpat.c      mpat  k 6
   #preprocess_family $R/bench/multiprodcon.c      multipc  workers 3   prods 3
   preprocess_family $R/bench/multiprodcon.c      multipc  workers 3   prods 4
   #preprocess_family $R/bench/multiprodcon.c      multipc  workers 4   prods 3
   preprocess_family $R/bench/multiprodcon.c      multipc  workers 4   prods 4
   #preprocess_family $R/bench/multiprodcon.c      multipc  workers 5   prods 3
   preprocess_family $R/bench/multiprodcon.c      multipc  workers 5   prods 4
   #preprocess_family $R/bench/multiprodcon.c      multipc  workers 6   prods 3
   preprocess_family $R/bench/multiprodcon.c      multipc  workers 6   prods 4
   #preprocess_family $R/bench/multiprodcon.c      multipc  workers 7   prods 3
   preprocess_family $R/bench/multiprodcon.c      multipc  workers 7   prods 4
   preprocess_family $R/bench/poke.c      poke  threads 3   iters 5
   preprocess_family $R/bench/poke.c      poke  threads 3   iters 6
   preprocess_family $R/bench/poke.c      poke  threads 4   iters 4
   preprocess_family $R/bench/poke.c      poke  threads 4   iters 5
   preprocess_family $R/bench/poke.c      poke  threads 4   iters 6
   preprocess_family $R/bench/poke.c      poke  threads 5   iters 4
   #preprocess_family $R/bench/poke.c      poke  threads 6   iters 3
   preprocess_family $R/bench/poke.c      poke  threads 6   iters 4
   preprocess_family $R/bench/poke.c      poke  threads 7   iters 3
   preprocess_family $R/bench/poke.c      poke  threads 7   iters 4
   preprocess_family $R/bench/poke.c      poke  threads 8   iters 3
   preprocess_family $R/bench/poke.c      poke  threads 8   iters 4

}

generate_bench_cesar ()
{
   # pre-conditions:
   # $R       - root of the cav18 folder

   #preprocess_family $R/bench/dispatcher.c dispatch   "serv" "4" "reqs" "`seq -w 1 7`"
   preprocess_family $R/bench/dispatcher.c dispatch   "serv" "3" "reqs" "`seq -w 2 3`"

   #preprocess_family $R/bench/dispatcher.c dispatch   "serv" "3" "reqs" "4"
   preprocess_family $R/bench/mpat.c       mpat       "k" "2"
   #preprocess_family $R/bench/spat.c       spat       "threads" "2 3" "mut" "2"

   #preprocess_family $R/bench/dispatcher.c dispatcher   "serv" "1 2 3 4 5 6 8" "reqs" "2 3"
   preprocess_family $R/bench/mpat.c       mpat         "k" "4 5"

   #preprocess_family $R/bench/poke.c       poke         "threads" "2 3 4" "iters" "2 3 4 5 6"
   #preprocess_family $R/bench/poke.c       poke         "threads" "5 6 7 8" "iters" "1 2 3 4"

   #preprocess_family $R/bench/poke.c       poke         "threads" "4" "iters" "4 6"
   #preprocess_family $R/bench/poke.c       poke         "threads" "5" "iters" "4"
   #preprocess_family $R/bench/poke.c       poke         "threads" "6" "iters" "4"
   #preprocess_family $R/bench/poke.c       poke         "threads" "7" "iters" "4"
   #preprocess_family $R/bench/poke.c       poke         "threads" "8" "iters" "4"

   #preprocess_family $R/bench/multiprodcon.c multipc     "workers" "3 4 5 6 7" "prods" "1 2 3 4 5"

   #preprocess_family $R/bench/ssb3.c       ssb3         "writers" "`seq -w 1 9`" "seqlen" "2 4 6 8"
   #preprocess_family $R/bench/ssbexp.c     ssbexp       "writers" "`seq -w 1 18`"
   #preprocess_family $R/bench/pi/pth_pi_mutex.c pi      "threads" "`seq -w 1 6`" "iters" "`seq -w 1000 2000 9000`"
}

generate_bench_skiplist ()
{
   # Selection of benchmarks for analyzing the performance of the sequential
   # trees. These are roughly a subset of those in generate_bench_selection
   # (Table 1) representative of the overall set of Table 1.

   preprocess_family $R/bench/dispatcher.c dispatch   "serv" "4" "reqs" "`seq -w 3 5`"
   preprocess_family $R/bench/mpat.c       mpat       "k" "`seq -w 4 6`"
   preprocess_family $R/bench/multiprodcon.c multipc  "prods" "3 4" "workers" "4"
   preprocess_family $R/bench/pi/pth_pi_mutex.c pi    "threads" "`seq -w 6 8`" "iters" "2000"
   preprocess_family $R/bench/poke.c       poke       "threads" "`seq -w 10 13`" "iters" "3"
}

runall_dpu ()
{
   # pre-conditions:
   # $TIMEOUT - a timeout specification valid for timeout(1)
   # $DPU     - path to the dpu tool to run

   OPTS="--mem 128M --stack 6M $DPU_OPTS"
   for i in *.i; do
      N=`echo "$i" | sed s/.i$//`

      if test $WANT_DPU_ALT_SDPOR = y; then
         # -k-1
         LOG=${N}_dpu_alt-1.txt
         CMD="$DPU $i -k-1 $OPTS"
         run_dpu
      fi

      if test $WANT_DPU_ALT0 = y; then
         # -k0
         LOG=${N}_dpu_alt0.txt
         CMD="$DPU $i -k0 $OPTS"
         run_dpu

         # if we got TO on -k0, surely we will also get it on -kX with X!=0
         if test "$WALLTIME" == "TO"; then continue; fi
      fi

      # k-partial
      for a in 1 2 3 4; do
         case $a in
         1) if test $WANT_DPU_ALT1 = n; then continue; fi;;
         2) if test $WANT_DPU_ALT2 = n; then continue; fi;;
         3) if test $WANT_DPU_ALT3 = n; then continue; fi;;
         4) if test $WANT_DPU_ALT4 = n; then continue; fi;;
         esac
         LOG=${N}_dpu_alt${a}.txt
         CMD="$DPU $i -k$a $OPTS"
         run_dpu

         # if we got 0 SSBs we skip higher -k
         if test "$SSBS" = 0; then break; fi
      done
   done
}

runall_nidhugg ()
{
   # pre-conditions:
   # $TIMEOUT - a timeout specification valid for timeout(1)
   # $NIDHUGG - path to the nidhugg tool to run

   if test $WANT_NIDHUGG = n; then return 0; fi

   OPTS="--c -sc
      -disable-mutex-init-requirement
      -extfun-no-race=printf
      -extfun-no-race=fprintf
      -extfun-no-race=write
      -extfun-no-race=exit
      -extfun-no-race=atoi
      -extfun-no-race=pow
      "
   OPTS=$(echo $OPTS) # remove newline characters

   for i in *.i; do
      N=`echo "$i" | sed s/.i$//`
      LOG=${N}_nidhugg.txt
      CMD="$NIDHUGG $OPTS $i"
      run_nidhugg
   done
}

dump_latex ()
{
   T=TABLE.tex
   echo "Generating latex table ..."

   echo "% This table has been automatically generated by runtable1.sh" > $T
   echo >> $T
   
   echo "% Benchmark                                                        DPU (k=1)              DUP (k=2)             DUP (k=3)             DUP (optimal)         Nidhugg" >> $T
   echo "% --------------------------------------------------------------- ---------------------- --------------------- --------------------- --------------------- ---------------------------------" >> $T
   echo "% Name                       LOC       Thrs      Confs     Events       Time       SSBs       Time       SSBs       Time       SSBs       Time        Mem        Time        Mem       SSBs" >> $T

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

      # check that all executions of dpu agree on the number of events
      EVENTS=$(cat ${i}_dpu* ${i}_nidhugg* | grep '^events ' | sort -u | grep -v 'events *-$')
      NUM=$(wc -l <<< "$EVENTS")
      if test "$NUM" -gt 1; then
         echo "WARNING: $i: tools report != number of events"
      fi
      EVENTS=$(head -n1 <<< "$EVENTS" | awk '{print $2}')

      # name, loc, numthreads, maxconfs, events
      ROW=$(printf '%-25s &  LOC & %8s & %8s & %8s' $i $NUMTHREADS $MAXCONFS $EVENTS)

      # columns for DPU
      for a in 1 2 3 0; do
         WTIME=$(cat ${i}_dpu_alt${a}.txt 2> /dev/null | grep '^walltime ' | sed 's/^walltime //')
         MAXRSS=$(cat ${i}_dpu_alt${a}.txt 2> /dev/null | grep '^maxrss ' | sed 's/^maxrss //')
         SSBS=$(cat ${i}_dpu_alt${a}.txt 2> /dev/null | grep '^SSBs ' | sed 's/^SSBs //')
         if test $a != 0; then
            # time, ssbs
            ROW="$ROW$(printf ' & %8s & %8s' $WTIME $SSBS)"
         else
            # time, memory
            ROW="$ROW$(printf ' & %8s & %8s' $WTIME $MAXRSS)"
         fi
      done

      # columns for NIDHUGG
      WTIME=$(cat ${i}_nidhugg.txt 2> /dev/null | grep '^walltime ' | sed 's/^walltime //')
      MAXRSS=$(cat ${i}_nidhugg.txt 2> /dev/null | grep '^maxrss ' | sed 's/^maxrss //')
      SSBS=$(cat ${i}_nidhugg.txt 2> /dev/null | grep '^SSBs ' | sed 's/^SSBs //')
      ROW="$ROW$(printf '  & %8s & %8s & %8s' $WTIME $MAXRSS $SSBS)"

      echo "$ROW" '\newrow' >> $T
   done

   echo "done!"
}

dry_run ()
{
   h2 "Dry running the tools"

   echo Running '``'$DPU --help'``'::
   echo
   $DPU --help 2>&1 | quote

   echo
   echo Running '``'$DPU --version'``'::
   echo
   $DPU --version 2>&1 | quote

   echo
   echo Running '``'$NIDHUGG --help'``'::
   echo
   $NIDHUGG --help 2>&1 | quote

   echo
   echo Running '``'$NIDHUGG --version'``'::
   echo
   $NIDHUGG --version 2>&1 | quote

   echo
   echo Running '``'clang-3.4 --version'`` (required by nidhugg)::'
   echo
   clang-3.4 --version 2>&1 | quote

   echo
   echo **WARNING**:
   echo If you see error messages above this line,
   echo then check that you understand what you are doing!!
}

get_tool_binaries ()
{
   h2 "Getting Tool Binaries"

   echo ::
   echo
   (
      mkdir tools
      #make -C $R/../../ dist
      #cp -Rv $R/../../dist/ tools/dpu
      #cp $HOME/x/devel/nidhugg/src/{nidhugg,nidhuggc} tools
      wget 'https://www.dropbox.com/s/p8leb9f9vkv3crr/cav18-tool-binaries.tar.gz'
      tar xzvf cav18-tool-binaries.tar.gz -C tools
   ) 2>&1 | quote

   DPU="tools/dpu/bin/dpu"
   NIDHUGG="tools/nidhuggc --nidhugg=tools/nidhugg"
}

main ()
{
   h1_date "Generation of Table 1"

   echo 'This is the output of the script ``runtable1.sh``.'
   echo 'It has been produced during the generation of the data for'
   echo 'Table 1 in the paper.'

   print_machine_infos
   get_tool_binaries
   dry_run

   h1_date "Preprocessing benchmark"
   echo ::
   echo
   #generate_bench_all 2>&1 | quote
   generate_bench_selection 2>&1 | quote #xxxxxxxxxxxxxxxxxxxxxxxxxxx
   #generate_bench_selection_below10s 2>&1 | quote
   #generate_bench_smallest 2>&1 | quote
   #generate_bench_cesar 2>&1 | quote
   #generate_bench_smallruntime 2>&1 | quote
   #generate_bench_morethan1sec 2>&1 | quote
   #generate_bench_skiplist 2>&1 | quote

   h1_date "Running tool DPU"
   echo ::
   echo
   runall_dpu 2>&1 | quote

   h1_date "Running tool NIDHUGG"
   echo ::
   echo
   runall_nidhugg 2>&1 | quote

   h1_date "Generating latex tables"
   echo ::
   echo
   dump_latex 2>&1 | quote

   echo
   echo
   echo End of the log.
   echo
   echo -n "Date: "
   date -R
}

R=logs.$(date +%F_%a_%T)
rm -Rf table1/logs table1/$R

mkdir -p table1/$R
cd table1
ln -s $R logs
cd $R

R=../.. # root of the cav18/ folder
main 2>&1 | tee OUTPUT.rst

