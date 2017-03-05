
R ?= ../..
D := $R/tests/unit

INC := -I $D/googletest/include
SRCS := $(wildcard $D/*.cc)
MSRCS := $D/unit-tests.cc
LIBS := $D/googletest/gtest_main.a

include $R/common.mk

all : run

run : compile
	$D/unit-tests

$D/googletest/gtest_main.a :
	make -C $D/googletest/

clean : clean_
clean_ :
	make -C $D/googletest/ clean
