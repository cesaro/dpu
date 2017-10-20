NAME = pth_pi_mutex

EXTRACTBC = extract-bc
STEROID = $(HOME)/steroid/utils/find-and-extract-bcs
DIS = llvm-dis
CC = wllvm
LIBS = -lpthread
DPU = $(shell pwd)/../../../dist/bin/dpu

all: ll

bc: $(NAME).bc
ll: $(NAME).ll

$(NAME): $(NAME).c
	$(CC) -O3 -o $@ $^ $(LIBS)

$(NAME).bc: $(NAME)
	$(EXTRACTBC) $^

$(NAME).full.bc: $(NAME).c
	$(STEROID)

$(NAME).ll: $(NAME).bc
	$(DIS) $^

run: $(NAME).ll
	$(DPU) $^ -vv -- 1 4 1024

clean:
	rm -f  $(NAME).full.bc $(NAME).ll $(NAME).bc $(NAME)

.PHONY: clean run
