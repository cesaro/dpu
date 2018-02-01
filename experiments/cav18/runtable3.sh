#!/bin/bash

# 1s = 1 second; 2m = 2 minutes; 3h = 3 hours
TIMEOUT=8m

WANT_DPU_ALT_SDPOR=n
WANT_DPU_ALT0=n
WANT_DPU_ALT1=n
WANT_DPU_ALT2=y
WANT_DPU_ALT3=n
WANT_DPU_ALT4=n
WANT_NIDHUGG=y

DPU_OPTS="-O1"
NIDHUGG_OPTS=

source ../cav18/runlib.sh

runall_dpu ()
{
   # pre-conditions:
   # $TIMEOUT - a timeout specification valid for timeout(1)
   # $DPU     - path to the dpu tool to run

   OPTS="--mem 128M --stack 1M $DPU_OPTS"
   for i in */*.c; do
      N=`echo "$i" | sed s/.c$//`

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

   for i in */*.c; do
      echo $N
      N=`echo "$i" | sed s/.c$//`
      LOG=${N}_nidhugg.txt
      CMD="$NIDHUGG $OPTS $i"
      run_nidhugg
   done
}

generate_bench_selection ()
{
   cp -R $R/../svcomp17/pthread .

   echo Benchmarks:
   ls -l */*.c
   echo
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

   #DPU=../../../../dist/bin/dpu
   #NIDHUGG="nidhuggc --nidhugg=nidhugg"
}

main ()
{
   h1_date "Generation of Table 3"

   echo 'This is the output of the script ``runsvcomp17.sh``.'
   echo 'It has been produced during the generation of the data for'
   echo 'Table 3 in the paper.'

   #print_machine_infos
   get_tool_binaries
   dry_run

   h1_date "Preprocessing benchmark"
   echo ::
   echo
   generate_bench_selection 2>&1 | quote

   h1_date "Running tool DPU"
   echo ::
   echo
   runall_dpu 2>&1 | quote


   h1_date "Running tool NIDHUGG"
   echo ::
   echo
   runall_nidhugg 2>&1 | quote

   echo
   echo
   echo End of the log.
   echo
   echo -n "Date: "
   date -R
}

R=logs.$(date +%F_%a_%T)
rm -Rf table3/logs table1/$R

mkdir -p table3/$R
cd table3
ln -s $R logs
cd $R

R=../.. # root of the cav18/ folder
main 2>&1 | tee OUTPUT.rst

