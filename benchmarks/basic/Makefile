CC=clang
CXX=clang++

CPPFLAGS=
CFLAGS=-Wall -std=c11
CXXFLAGS=-Wall -std=c++11
LDLIBS=-lpthread

SRCS:=$(wildcard *.c)
TARGETS:=$(SRCS:.c=)

all: $(TARGETS)

clean :
	rm -f $(TARGETS)

%.ll : %.c
	$(CC) -S -emit-llvm $(CFLAGS_) $< -o $@
%.bc : %.c
	$(CC) -c -emit-llvm $(CFLAGS_) $< -o $@
%.ll : %.cc
	$(CC) -S -emit-llvm $(CXXFLAGS_) $< -o $@
%.bc : %.cc
	$(CC) -c -emit-llvm $(CXXFLAGS_) $< -o $@

