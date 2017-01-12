
# Copyright (C) 2010-2016  Cesar Rodriguez <cesar.rodriguez@lipn.fr>
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the Free
# Software Foundation, either version 3 of the License, or any later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
# more details.
#
# You should have received a copy of the GNU General Public License along with
# this program.  If not, see <http://www.gnu.org/licenses/>.

include defs.mk

.PHONY: fake all g test clean distclean prof dist compile tags run dot
	

ifeq ($(shell id -nu),cesar)
all : compile run
else
all : compile
endif

compile: $(TARGETS)

run: compile input.ll
	./src/main
	make dot

input.ll : program.ll ../steroid/rt/rt.ll
	llvm-link-$(LLVMVERS) -S $^ -o $@


#program.ll : /tmp/cunf3.ll
#program.ll : ../steroid/tests/hello.ll
#program.ll : benchmarks/basic/hello.ll
#program.ll : benchmarks/basic/cjlu.ll
#program.ll : benchmarks/basic/huyen.ll

ifeq ($(shell id -nu),cesar)
program.ll : benchmarks/basic/cjlu.ll
else
program.ll : benchmarks/basic/cjlu.ll
endif
	opt-3.7 -S -O3 -mem2reg $< > $@
	#opt-3.7 -S -verify $< > $@

$(TARGETS) : % : %.o $(OBJS)
	@echo "LD  $@"
	@$(CXX) $(LDFLAGS) -o $@ $^ ../steroid/src/libsteroids.a $(LDLIBS)
    
#$(MINISAT)/build/release/lib/libminisat.a :
#	cd $(MINISAT); make lr

prof : $(TARGETS)
	rm gmon.out.*
	src/main /tmp/ele4.ll_net

tags : $(SRCS)
	ctags -R --c++-kinds=+p --fields=+K --extra=+q src/ $(shell llvm-config-$(LLVMVERS) --includedir)

g gdb : $(TARGETS)
	gdb ./src/main
		
vars :
	@echo "CC       $(CC)"
	@echo "CXX      $(CXX)"
	@echo "CFLAGS   $(CFLAGS)"
	@echo "CPPFLAGS $(CPPFLAGS)"
	@echo "CXXFLAGS $(CXXFLAGS)"
	@echo "LDFLAGS  $(LDFLAGS)"
	@echo "LDLIBS   $(LDLIBS)"
	@echo "TARGETS  $(TARGETS)"
	@echo "MSRCS    $(MSRCS)"
	@echo "MOBJS    $(MOBJS)"
	@echo "SRCS     $(SRCS)"
	@echo "OBJS     $(OBJS)"
	@echo "DEPS     $(DEPS)"
	@echo "DOTPNG   $(DOTPNG)"

clean :
	@rm -f $(TARGETS) $(MOBJS) $(OBJS)
	@rm -f dot/*.dot
	@rm -f dot/*.png
	@rm -f dot/*.pdf
	@echo Cleaning done.

distclean : clean
	@rm -f $(DEPS)
	@rm -Rf dist/
	@echo Mr. Proper done.

dist : all
	rm -Rf dist/
	mkdir dist/
	mkdir dist/bin
	mkdir dist/lib
	mkdir dist/examples
	mkdir dist/examples/corbett
	mkdir dist/examples/dekker
	mkdir dist/examples/dijkstra
	cp src/cunf/cunf dist/bin/cunf
	cp src/pep2dot dist/bin
	cp src/pep2pt dist/bin
	cp tools/cna dist/bin
	cp tools/grml2pep.py dist/bin
	cp tools/cuf2pep.py dist/bin
	#cp minisat/core/minisat dist/bin
	cp -R tools/ptnet dist/lib
	cp -R examples/cont dist/examples/corbett/
	cp -R examples/other dist/examples/corbett/
	cp -R examples/plain dist/examples/corbett/
	cp -R examples/pr dist/examples/corbett/
	for i in 02 04 05 08 10 20 30 40 50; do ./tools/mkdekker.py $$i > dist/examples/dekker/dek$$i.ll_net; done
	for i in 02 03 04 05 06 07; do ./tools/mkdijkstra.py $$i > dist/examples/dijkstra/dij$$i.ll_net; done
	
dot: $(SVGS)

o open :
	(ls dot/*.svg | head -n1 | xargs eog) &
 	     
-include $(DEPS)

