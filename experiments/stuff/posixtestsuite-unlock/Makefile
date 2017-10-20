EXEC = 2-1
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

2-1: 2-1.c
	$(CC) -O3 -o $@ $^ $(LIBS)

3-1: 3-1.c
	$(CC) -O3 -o $@ $^ $(LIBS)

5-1: 5-1.c
	$(CC) -O3 -o $@ $^ $(LIBS)

5-2: 5-2.c
	$(CC) -O3 -o $@ $^ $(LIBS)

1-1.bc: 1-1
	$(EXTRACTBC) $^

2-1.bc: 2-1
	$(EXTRACTBC) $^

3-1.bc: 3-1
	$(EXTRACTBC) $^

5-1.bc: 5-1
	$(EXTRACTBC) $^

5-2.bc: 5-2
	$(EXTRACTBC) $^

%.ll: %.bc
	$(DIS) $^

%.full.bc: $(BC)
	$(STEROID)

run: $(FULL)
	$(DPU) $< -vv
	$(DPU) $(word 2,$^) -vv
	$(DPU) $(word 3,$^) -vv
	$(DPU) $(word 4,$^) -vv
	$(DPU) $(word 5,$^) -vv

clean:
	rm -f $(FULL) $(EXEC) $(BC) $(LL)

.PHONY: clean run


