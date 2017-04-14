
all :
	true

nohup1 :
	nohup ./runtable1.sh > nohup.out 2>&1 &
nohup2 :
	nohup ./runtable2.sh > nohup.out 2>&1 &
nohup12 :
	nohup bash -c './runtable1.sh; ./runtable2.sh' > nohup.out 2>&1 &

clean :
	rm -f latest.table[12]
	rm -Rf table1.*
	rm -Rf table2.*
