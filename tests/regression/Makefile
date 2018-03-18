
R ?= ../..
D := $R/tests/regression

all : run

run :
	$D/regtest.sh $R/dist/bin/dpu $D

clean : clean
	@rm -f $D/regression.log*
	@find $D | grep '.log$$' | xargs rm -f

realclean :
