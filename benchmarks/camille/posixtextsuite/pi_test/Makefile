EXEC = pitest-1  pitest-2  pitest-3  pitest-4  pitest-5  pitest-6
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


pitest-1: pitest-1.c
	$(CC) -O3 -o $@ $^ $(LIBS)

pitest-2: pitest-2.c
	$(CC) -O3 -o $@ $^ $(LIBS)

pitest-3: pitest-3.c
	$(CC) -O3 -o $@ $^ $(LIBS)

pitest-4: pitest-4.c
	$(CC) -O3 -o $@ $^ $(LIBS)

pitest-5: pitest-5.c
	$(CC) -O3 -o $@ $^ $(LIBS)

pitest-6: pitest-6.c
	$(CC) -O3 -o $@ $^ $(LIBS)

pitest-1.bc: pitest-1
	$(EXTRACTBC) $^

pitest-2.bc: pitest-2
	$(EXTRACTBC) $^

pitest-3.bc: pitest-3
	$(EXTRACTBC) $^

pitest-4.bc: pitest-4
	$(EXTRACTBC) $^

pitest-5.bc: pitest-5
	$(EXTRACTBC) $^

pitest-6.bc: pitest-6
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
	$(DPU) $(word 6,$^) -vv

clean:
	rm -f $(FULL) $(EXEC) $(BC) $(LL)

.PHONY: clean run


