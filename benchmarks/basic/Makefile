VERS=3.7
CC=clang-$(VERS)
CXX=clang++-$(VERS)

CPPFLAGS=
CFLAGS=-Wall -O1
CXXFLAGS=-Wall -O1
LDLIBS=-lpthread

SRCS:=$(wildcard *.c)
BINS:=$(SRCS:.c=)
LLS:=$(SRCS:.c=.ll)

all: $(LLS)
all-bins : $(BINS)

clean :
	rm -f $(BINS) $(LLS)

%.ll : %.c
	#$(CC) -S -emit-llvm -include __dpu.h $(CFLAGS) $< -o $@
	$(CC) -S -emit-llvm $(CFLAGS) $< -o $@
%.bc : %.c
	$(CC) -c -emit-llvm -include __dpu.h $(CFLAGS) $< -o $@
%.ll : %.cc
	$(CC) -S -emit-llvm -include __dpu.h $(CXXFLAGS) $< -o $@
%.bc : %.cc
	$(CC) -c -emit-llvm -include __dpu.h $(CXXFLAGS) $< -o $@
%.s : %.bc
	llc-$(VERS) $< -o $@
