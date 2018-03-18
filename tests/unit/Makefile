
R ?= ../..
D := $R/tests/unit

INC := -I $D -I $D/googletest/include -I $R/src
SRCS := $(wildcard $D/*.cc $D/*/*.cc) $(wildcard $R/src/*.cc $R/src/*.c $R/src/*/*c $R/src/*/*.cc)
SRCS := $(filter-out %/src/main.cc, $(SRCS))
MSRCS := $D/unit-tests.cc
LIBS := $D/googletest/gtest_main.a

include $R/common.mk

all : run

run : compile
	$D/unit-tests

$D/googletest/gtest_main.a $D/googletest/gtest.a :
	make -C $D/googletest/

clean : clean_
clean_ :
	@make -C $D/googletest/ clean
