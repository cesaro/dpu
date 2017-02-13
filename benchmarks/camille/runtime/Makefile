EXEC = uncontested_mutex_lock contested_mutex_lock
FULL = $(addsuffix .full.bc, $(EXEC))

all: $(FULL)

EXTRACTBC = extract-bc
STEROID = $(HOME)/steroid/utils/find-and-extract-bcs
DIS = llvm-dis
CC = wllvm
LIBS = -lpthread
DPU = $(shell pwd)/../../../dist/bin/dpu

BC = $(addsuffix .bc, $(EXEC))
bc: $(BC)
LL = $(addsuffix .ll, $(EXEC))
ll: $(LL)
SOURCES = $(addsuffix .c, $(EXEC))


uncontested_mutex_lock: uncontested_mutex_lock.c
	$(CC) -O3 -o $@ $^ $(LIBS)

contested_mutex_lock: contested_mutex_lock.c
	$(CC) -O3 -o $@ $^ $(LIBS)

uncontested_mutex_lock.bc: uncontested_mutex_lock
	$(EXTRACTBC) $^

contested_mutex_lock.bc: contested_mutex_lock
	$(EXTRACTBC) $^

%.ll: %.bc
	$(DIS) $^

%.full.bc: $(BC)
	$(STEROID)

run: $(FULL)
	$(DPU) $< -vv
	$(DPU) echo $(word 2,$^) -vv

clean:
	rm -f $(FULL) $(EXEC) $(BC) $(LL)

.PHONY: clean run


