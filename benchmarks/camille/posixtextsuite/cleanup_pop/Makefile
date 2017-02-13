EXEC = 1-1  1-2  1-3
FULL = $(addsuffix .full.bc, $(EXEC))

all: $(FULL)

EXTRACTBC = extract-bc
STEROID = $(HOME)/steroid/utils/find-and-extract-bcs
DIS = llvm-dis
CC = wllvm
LIBS = -lpthread
DPU = $(HOME)/dpu2/dist/bin/dpu

BC = $(addsuffix .bc, $(EXEC))
bc: $(BC)
LL = $(addsuffix .ll, $(EXEC))
ll: $(LL)
SOURCES = $(addsuffix .c, $(EXEC))

1-1: 1-1.c
	$(CC) -O3 -o $@ $^ $(LIBS)

1-2: 1-2.c
	$(CC) -O3 -o $@ $^ $(LIBS)

1-3: 1-3.c
	$(CC) -O3 -o $@ $^ $(LIBS)

1-1.bc: 1-1
	$(EXTRACTBC) $^

1-2.bc: 1-2
	$(EXTRACTBC) $^

1-3.bc: 1-3
	$(EXTRACTBC) $^

%.ll: %.bc
	$(DIS) $^

%.full.bc: $(BC)
	$(STEROID)

run: $(FULL)
	$(DPU) $< -vv
	$(DPU) $(word 2,$^) -vv
	$(DPU) $(word 3,$^) -vv

clean:
	rm -f $(FULL) $(EXEC) $(BC) $(LL)

.PHONY: clean run


