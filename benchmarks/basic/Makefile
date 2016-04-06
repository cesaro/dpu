CC=clang
CXX=clang++

CPPFLAGS=
CFLAGS=-Wall -O1
CXXFLAGS=-Wall -O1
LDLIBS=-lpthread

SRCS:=$(wildcard *.c)
TARGETS:=$(SRCS:.c=)

all: $(TARGETS)

clean :
	rm -f $(TARGETS)

%.ll : %.c
	$(CC) -S -emit-llvm -include __dpu.h $(CFLAGS) $< -o $@
%.bc : %.c
	$(CC) -c -emit-llvm -include __dpu.h $(CFLAGS) $< -o $@
%.ll : %.cc
	$(CC) -S -emit-llvm -include __dpu.h $(CXXFLAGS) $< -o $@
%.bc : %.cc
	$(CC) -c -emit-llvm -include __dpu.h $(CXXFLAGS) $< -o $@

