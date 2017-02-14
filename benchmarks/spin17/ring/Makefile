EXEC = c-ring
FULL = $(EXEC).bc

all: $(FULL)

EXTRACTBC = extract-bc
STEROID = $(HOME)/steroid/utils/find-and-extract-bcs
DIS = llvm-dis
CC = wllvm -Wno-int-to-void-pointer-cast -DSTATIC_TH_STACK
LIBS = -lpthread
DPU = $(HOME)/dpu2/dist/bin/dpu

BC = $(addsuffix .bc, $(EXEC))
bc: $(BC)
LL = $(addsuffix .ll, $(EXEC))
ll: $(LL)
SOURCES = $(addsuffix .c, $(EXEC))

c-ring: c-ring.c
	$(CC) -O3 -o $@ $^ $(LIBS)

c-ring.bc: c-ring
	$(EXTRACTBC) $^

run: $(FULL)
	$(DPU) $< -vv -- 1 50 12

clean:
	rm -f $(FULL) $(EXEC) $(BC) $(LL)

.PHONY: clean run


