EXEC = sem_philosopher
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

sem_conpro: sem_conpro.c
	$(CC) -O3 -o $@ $^ $(LIBS)

sem_philosopher: sem_philosopher.c
	$(CC) -O3 -o $@ $^ $(LIBS)

sem_sleepingbarber: sem_sleepingbarber.c
	$(CC) -O3 -o $@ $^ $(LIBS)

sem_conpro.bc: sem_conpro
	$(EXTRACTBC) $^

sem_philosopher.bc: sem_philosopher
	$(EXTRACTBC) $^

sem_sleepingbarber.bc: sem_sleepingbarber
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


