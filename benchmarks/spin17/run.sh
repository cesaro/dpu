#!/bin/bash


# ==== BEGIN CONFIGURATION VARIABLES ====

DPU=dpu

NIDHUGG=/usr/local/bin/nidhuggc \
    --nidhugg=/usr/local/bin/nidhugg \
    -extfun-no-race=printf \
    -extfun-no-race=write \
    -extfun-no-race=exit \
    -extfun-no-race=atoi

# 1s = 1 second; 2m = 2 minutes; 3h = 3 hours
TIMEOUT=2s

# ==== END CONFIGURATION VARIABLES ====





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
         CMD="$CPP -E -D PARAM1=$P1_ $CFILE -o ${IPATH}_${P1NAME}=${P1}.i"
         echo $CMD
         $CMD
      done
      return
   fi

   # 2 parameters
   for P1 in `echo $P1VALS`; do
      for P2 in `echo $P2VALS`; do
         P1_=$(echo "$P1" | sed 's/^0\+//') # remove trailing 0s
         P2_=$(echo "$P1" | sed 's/^0\+//') # remove trailing 0s
         CMD="$CPP -E -D PARAM1=$P1_ -D PARAM2=$P2_ $CFILE -o ${IPATH}_${P1NAME}=${P1}_${P2NAME}=${P2}.i"
         echo $CMD
         $CMD
      done
   done
}

generate_bench ()
{
   preprocess_family dispatcher.c cpp/dispatcher   "srvs" "1 2 3 4 5 6" "reqs" "1 2 3 4"
   preprocess_family mpat.c       cpp/mpat         "k" "`seq -w 1 20`"
   preprocess_family poke.c       cpp/poke         "thrs" "1 2 3 4 5 6" "iters" "1 2 3 4 5 6 7 8 9"
   preprocess_family spat.c       cpp/spat         "thrs" "1 2 3 4 5 6" "mut" "1 2 3 4 5"
   preprocess_family ssb3.c       cpp/ssb3         "writers" "`seq -w 1 15`" "seqlen" "2 4 6 8"
   preprocess_family ssbexp.c     cpp/ssbexp       "writers" "`seq -w 1 25`"
   preprocess_family pi/pth_pi_mutex.c cpp/pi      "thrs" "`seq -w 1 6`" "iters" "`seq -w 1000 2000 9000`"
}

round() {
    python -c "print '%.3f' % (float ($1) / (1000 * 1000 * 1000))"
}

run_dpu ()
{
   for i in cpp/*.i; do
      N=`echo "$i" | sed s/.i$//`
      for a in -1 0 1 2; do
         LOG=${N}_dpu_alt=${a}.log
         CMD="time timeout $TIMEOUT $DPU $i -a $a"
         echo "name      $N" > $LOG
         echo "cmd       $CMD" >> $LOG
         echo "alt       $a" >> $LOG

         # run the program
         echo $CMD
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
            if grep -q "dpu: error.*unhandled" ${LOG}.{stdout,stderr}; then
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
         printf 'walltime=%-8s max-confs=%-4s ssbs=%-5s events=%-5s\n' $WALLTIME $MAXCONFS $SSBS $EVENTS

         echo -e "\n\nstdout:" >> $LOG
         cat ${LOG}.stdout >> $LOG
         echo -e "\nstderr:" >> $LOG
         cat ${LOG}.stderr >> $LOG
         rm ${LOG}.stdout
         rm ${LOG}.stderr
      done
   done
}

run_nidhugg ()
{
   for i in cpp/*.i; do
      N=`echo "$i" | sed s/.i$//`
      LOG=${N}_nidhugg.log
      CMD="time timeout $TIMEOUT $NIDHUGG $i"
      echo "name      $i" > $LOG
      echo "cmd       $CMD" >> $LOG

      # run the program
      echo $CMD
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
      printf 'walltime=%-8s max-confs=%-4s ssbs=%-5s\n' $WALLTIME $MAXCONFS $SSBS

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
   # in a loop, scan all .i files in cpp/
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
   echo " run.sh gen    Generates the bemchmarks (see folder cpp/)"
   echo " run.sh run    Assumes that BLA "
}

main ()
{
   rm -Rf cpp/
   mkdir cpp

   generate_bench
   run_dpu
   run_nidhugg
   dump_latex
}

main
