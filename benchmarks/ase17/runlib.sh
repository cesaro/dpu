
round() {
    python -c "print '%.3f' % (float ($1) / (1000 * 1000 * 1000))"
}

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

