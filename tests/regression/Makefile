
R ?= ../..
D := tests/regression
TARGETS :=

include $R/common.mk

all : run

run :
	./regtest.sh $R/dist/bin/dpu .

clean : clean_
clean_ :
	rm -f regression.log*
	find . | grep '.log$$' | xargs rm -f
