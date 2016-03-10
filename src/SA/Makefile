CC = clang
CPP = clang++
LD = clang++

INCLUDEDIR =  -I/usr/include/llvm-3.5 -I/usr/include/llvm-c-3.5
MACROSDEF = -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS
LIBDIR = -L/usr/lib/llvm-3.5/lib
LDLINK = -lLLVM-3.5
STDDEF = -std=c++11
CPPFLAGS =
CLFAGS =

SASRC = sa.cc
SAOBJ = $(SASRC:.cc=.o)
EXEC = sa

#all: $(EXEC)
all: run

$(EXEC): $(SAOBJ)
	$(LD) $(LDFLAGS) -o $@ $< $(LIBDIR)  $(LDLINK)

%.o: %.cc
	$(CPP) $(CPPFLAGS) -c $< $(INCLUDEDIR) $(MACROSDEF) $(STDDEF) 

test: test.c
	$(CC) -o $@ $<

test.ll: test.c
	$(CC) -S -emit-llvm $< -o $@

.PHONY: run clean

run: $(EXEC) test.ll
	./$(EXEC) test.ll

clean:
	@rm -f test sa *.o
