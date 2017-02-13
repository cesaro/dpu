EXEC = c-ring c-ring2 c-ring3 c-ring4
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

c-ring: c-ring.c
	$(CC) -O3 -o $@ $^ $(LIBS)

c-ring2: c-ring2.c
	$(CC) -O3 -o $@ $^ $(LIBS)

c-ring3: c-ring3.c
	$(CC) -O3 -o $@ $^ $(LIBS)

c-ring4: c-ring4.c
	$(CC) -O3 -o $@ $^ $(LIBS)

c-ring.bc: c-ring
	$(EXTRACTBC) $^

c-ring2.bc: c-ring2
	$(EXTRACTBC) $^

c-ring3.bc: c-ring3
	$(EXTRACTBC) $^

c-ring4.bc: c-ring4
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

clean:
	rm -f $(FULL) $(EXEC) $(BC) $(LL)

.PHONY: clean run


