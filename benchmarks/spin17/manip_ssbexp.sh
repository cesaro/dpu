#!/bin/bash


FILENAME=ssbexp.c
EXTRACTBC=extract-bc
DPU=${HOME}/dpu2/dist/bin/dpu
NIDHUGG=./nidhugg.sh
WLLVM=wllvm
LIBS="-lpthread"
COPTS="-O3"
LLVMDIS=llvm-dis

round() {
    echo "scale=3 ; $1/1000000000" | bc
}

function runtest {
    # generate the .ll file
    sed "s/XXXX/$1/" ${FILENAME} > generated.c
    $WLLVM $COPTS -o generated generated.c $LIBS
    $EXTRACTBC generated
    $LLVMDIS generated.bc

    LINENAME="SSBexp & $(($1+1)) & "
    
    # run DPU experiments
    for ALT in 0 1 2 3 ; do
	echo -n $LINENAME
	BEGIN=`date +%s%N`
	$DPU generated.ll -a${ALT} 2&>1 > trace 
	END=`date +%s%N`
	EXECTIME=`round $(($END-$BEGIN))`
	echo -n $ALT " & " $EXECTIME " & "
	AVG=`grep 'average max trail size' trace | awk '{print $4 " & " }'`
	STAT=`grep 'summary' trace | awk '{print $3 " & " $5 " & " $7 " & " }'`
	echo -n $STAT " " $AVG

	if [ $ALT -eq 0  ] ; then
	    # run Nihugg experiment
	    BEGIN=`date +%s%N`
	    $NIDHUGG -sc generated.ll > trace 2> log
	    END=`date +%s%N`
	    EXECTIME=`round $(($END-$BEGIN))`
	    STAT=`grep "Trace" trace | awk '{print $3 " & " $5}'`
	    echo -n $EXECTIME " & " $STAT
	    echo "\\\\"
	else
	    echo "&&\\\\"
	fi
    done

}

for TH in 1 2 3 4 5 6 8; do
    runtest $TH
done
