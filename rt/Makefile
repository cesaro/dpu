
.PHONY : compile_

R ?= ..
D := $R/rt
TARGETS :=

include $R/common.mk

all : compile

compile : $D/verifier.bc

clean : clean_

clean_ :
	@rm -f $D/verifier.bc
