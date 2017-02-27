#!/bin/bash

# ==== BEGIN CONFIGURATION VARIABLES ====

# 1s = 1 second; 2m = 2 minutes; 3h = 3 hours
TIMEOUT=15s

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
else
   DPU=../../dist/bin/dpu
   NIDHUGG=nidhugg
fi


preprocess_family()
{
   CFILE=$1
   IPATH=$2
   P1NAME=$3
   P1VALS=$4
   P2NAME=$5
   P2VALS=$6
   CPP=cpp

   # only 1 parameter
   if test -z "$P2NAME"; then
      for P1 in `echo $P1VALS`; do
         P1_=$(echo "$P1" | sed 's/^0\+//') # remove trailing 0s
         CMD="$CPP -E -D PARAM1=$P1_ $CFILE -o ${IPATH}-${P1NAME}${P1}.i"
         echo $CMD
         $CMD
      done
      return
   fi

   # 2 parameters
   for P1 in `echo $P1VALS`; do
      for P2 in `echo $P2VALS`; do
         P1_=$(echo "$P1" | sed 's/^0\+//') # remove trailing 0s
         P2_=$(echo "$P2" | sed 's/^0\+//') # remove trailing 0s
         CMD="$CPP -E -D PARAM1=$P1_ -D PARAM2=$P2_ $CFILE -o ${IPATH}-${P1NAME}${P1}_${P2NAME}${P2}.i"
         echo $CMD
         $CMD
      done
   done
}

generate_bench ()
{
   preprocess_family ../dispatcher.c dispatcher   "serv" "`seq -w 1 12`" "reqs" "1 2 3 4 5 6 8 10 12"
   preprocess_family ../mpat.c       mpat         "k" "`seq -w 1 7`"
   preprocess_family ../poke.c       poke         "threads" "1 2 3" "iters" "`seq -w 1 2 15`"
   preprocess_family ../spat.c       spat         "threads" "1 2 3 4 5 6" "mut" "1 2 3 4 5"
   preprocess_family ../ssb3.c       ssb3         "writers" "`seq -w 1 9`" "seqlen" "2 4 6 8"
   preprocess_family ../ssbexp.c     ssbexp       "writers" "`seq -w 1 18`"
   preprocess_family ../pi/pth_pi_mutex.c pi      "threads" "`seq -w 1 6`" "iters" "`seq -w 1000 2000 9000`"

   # PROPOSAL OF MARCELO: ???
   #preprocess_family ../dispatcher.c dispatcher   "serv" "`seq -w 1 10`" "reqs" "1 2 3 4 5 6 8 10 12 13 14"
   #preprocess_family ../mpat.c       mpat         "k" "`seq -w 1 7`"
   #preprocess_family ../poke.c       poke         "threads" "`seq -w 1 7`" "iters" "`seq -w 1 2 15`"
   #preprocess_family ../spat.c       spat         "threads" "1 2 3 4 5 6" "mut" "1 2 3 4 5"
   #preprocess_family ../ssb3.c       ssb3         "writers" "`seq -w 1 9`" "seqlen" "2 4 6 8"
   #preprocess_family ../ssbexp.c     ssbexp       "writers" "`seq -w 1 18`"
   #preprocess_family ../pi/pth_pi_mutex.c pi      "threads" "`seq -w 1 6`" "iters" "`seq -w 1000 2000 9000`"
}

generate_bench_smallest ()
{
   preprocess_family ../dispatcher.c dispatcher   "serv" "2 3" "reqs" "3 4"
   preprocess_family ../mpat.c       mpat         "k" "`seq -w 2 6`"
   preprocess_family ../poke.c       poke         "th" "1 2 3" "iters" "`seq -w 1 2 6`"
   preprocess_family ../spat.c       spat         "threads" "2 3 4" "mut" "2 3"
   preprocess_family ../ssb3.c       ssb3         "writers" "`seq -w 2 5`" "seqlen" "4 6"
   preprocess_family ../ssbexp.c     ssbexp       "writers" "`seq -w 2 5`"
   preprocess_family ../pi/pth_pi_mutex.c pi      "threads" "`seq -w 1 3`" "iters" "`seq -w 1000 2000 9000`"
}

round() {
    python -c "print '%.3f' % (float ($1) / (1000 * 1000 * 1000))"
}

run_dpu ()
{
   for i in *.i; do
      N=`echo "$i" | sed s/.i$//`
      for a in -1 0 1 2; do
         LOG=${N}_dpu_alt${a}.log
         CMD="time timeout $TIMEOUT $DPU $i -a $a --mem 128M --stack 6M -v"
         echo "name      $N" > $LOG
         echo "cmd       $CMD" >> $LOG
         echo "alt       $a" >> $LOG

         # run the program
         echo "$CMD"
         echo "> $LOG"
         BEGIN=`date +%s%N`
         $CMD > ${LOG}.stdout 2> ${LOG}.stderr
         EXITCODE=$?
         END=`date +%s%N`

         # check for timeouts
         if test "$EXITCODE" = 124; then
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
            MAXCONFS="-"
            SSBS="-"
            EVENTS="-"
         else
            WALLTIME=`round $(($END-$BEGIN))`
            MAXCONFS=$(grep "dpu: summary" ${LOG}.stdout | awk '{print $3}')
            SSBS=$(grep "dpu: summary" ${LOG}.stdout | awk '{print $5}')
            EVENTS=$(grep "dpu: summary" ${LOG}.stdout | awk '{print $7}')
         fi

         # dump numbers to the log file
         echo "" >> $LOG
         echo "exitcode  $EXITCODE" >> $LOG
         echo "begin     $BEGIN" >> $LOG
         echo "end       $END" >> $LOG
         echo "walltime  $WALLTIME" >> $LOG
         echo "maxconfs  $MAXCONFS" >> $LOG
         echo "SSBs      $SSBS" >> $LOG
         echo "events    $EVENTS" >> $LOG

         # print a summary
         printf 'WTIME=%-8s MAXCONF=%-4s SSBS=%-5s EVENTS=%-5s\n\n' $WALLTIME $MAXCONFS $SSBS $EVENTS

         echo -e "\n\nstdout:" >> $LOG
         cat ${LOG}.stdout >> $LOG
         echo -e "\nstderr:" >> $LOG
         cat ${LOG}.stderr >> $LOG
         rm ${LOG}.stdout
         rm ${LOG}.stderr

         # if a >= 1 and we got 0 SSBs, no need to run with larger a
         #if test \( $a -ge 1 \) -a \( "$SSBS" = 0 \); then break; fi
      done
   done
}

run_nidhugg ()
{
   for i in *.i; do
      N=`echo "$i" | sed s/.i$//`
      LOG=${N}_nidhugg.log
      CMD="time timeout $TIMEOUT $NIDHUGG $i"
      echo "name      $i" > $LOG
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
         WALLTIME="TO"
         MAXCONFS="-"
         SSBS="-"
      elif test "$EXITCODE" != 0; then
         WALLTIME="ERR"
         MAXCONFS="-"
         SSBS="-"
      else
         WALLTIME=`round $(($END-$BEGIN))`
         MAXCONFS=$(grep 'Trace count: ' ${LOG}.stdout | sed 's/.*Trace count: //' | awk '{print $1}')
         SSBS=$(grep 'Trace count: ' ${LOG}.stdout | sed 's/.*Trace count: //' | awk '{print $3}')
      fi

      # dump numbers to the log file
      echo "" >> $LOG
      echo "exitcode  $EXITCODE" >> $LOG
      echo "begin     $BEGIN" >> $LOG
      echo "end       $END" >> $LOG
      echo "walltime  $WALLTIME" >> $LOG
      echo "maxconfs  $MAXCONFS" >> $LOG
      echo "SSBs      $SSBS" >> $LOG

      # print a summary
      printf 'WTIME=%-8s MAXCONF=%-4s SSBS=%-5s\n\n' $WALLTIME $MAXCONFS $SSBS

      echo -e "\n\nstdout:" >> $LOG
      cat ${LOG}.stdout >> $LOG
      echo -e "\nstderr:" >> $LOG
      cat ${LOG}.stderr >> $LOG
      rm ${LOG}.stdout
      rm ${LOG}.stderr
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

   echo
   echo "$NIDHUGG --help"
   $NIDHUGG --help

   echo
   echo If you see errors above this line,
   echo then check that you understand what you are doing.
   echo
}

main ()
{
   test_can_run
   generate_bench
   #generate_bench_smallest
   run_dpu
   run_nidhugg
   dump_latex
}


R=logs.$(date -R | sed -e 's/ /_/g' -e 's/[,+]//g' -e 's/:/-/g')
rm -Rf $R latest
mkdir $R
ln -s $R latest
cd $R

main 2>&1 | tee XXX.log

