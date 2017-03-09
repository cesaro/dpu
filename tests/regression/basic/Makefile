CFLAGS = -Wall -pthread
CC=gcc

TARGETS = $(patsubst %.c, %, $(wildcard *.c))

all : $(TARGETS)

clean :
	rm -f $(TARGETS) regression.log* *.log
