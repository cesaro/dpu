
.PHONY : table1 table2

all :
	@echo 'Error: Read the Makefile and choose a target! :)'

table1 :
	./runtable1.sh
table2 :
	./runtable2.sh
table3 :
	./runtable3.sh

nohup1 :
	nohup ./runtable1.sh > nohup.out 2>&1 &
nohup2 :
	nohup ./runtable2.sh > nohup.out 2>&1 &
nohup12 :
	nohup bash -c './runtable1.sh; ./runtable2.sh' > nohup.out 2>&1 &

clean :
	rm -Rf table1/log*
	rm -Rf table2/log*
	rm -Rf table3/log*
