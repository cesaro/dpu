CC = clang
CPP = clang++
LD = clang++

LLVMCXXFLAGS=$(shell llvm-config --cppflags)
LLVMLDFLAGS=$(shell llvm-config --ldflags)
LLVMLIBS=$(shell llvm-config --libs all)

LIBDIR = $(LLVMLDFLAGS)
#LDLINK = -lLLVM-3.5
LDLINK = $(LLVMLIBS) -lpthread -ldl -lncurses
STDDEF = -std=c++11
CPPFLAGS =
CLFAGS =

SASRC = sa.cc sa_utils.cc
SAOBJ = $(SASRC:.cc=.o)
EXEC = sa

TESTFILE = test_thread3

all: $(EXEC)
#all: run

$(EXEC): $(SAOBJ)
	@$(LD) $(LDFLAGS) -o $@ $(SAOBJ) $(LIBDIR)  $(LDLINK)

%.o: %.cc
	$(CPP) $(CPPFLAGS) -c $< $(LLVMCXXFLAGS) $(STDDEF) 

$(TESTFILE): $(TESTFILE).c
	$(CC) -o $@ $<

%.ll : %.c
	$(CC) -S -emit-llvm $(CFLAGS) $< -o $@
%.bc : %.c
	$(CC) -c -emit-llvm $(CFLAGS) $< -o $@
%.ll : %.cc
	$(CC) -S -emit-llvm $(CXXFLAGS) $< -o $@
%.bc : %.cc
	$(CC) -c -emit-llvm $(CXXFLAGS) $< -o $@

.PHONY: run clean

run: $(EXEC) $(TESTFILE).ll
	./$(EXEC) $(TESTFILE).ll

clean:
	@rm -f test sa *.o *.ll
