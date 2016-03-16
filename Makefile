
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

.PHONY: fake all g test clean distclean prof dist compile

all : compile

compile: $(TARGETS) tags

r run: compile
	./src/main | tee out

$(TARGETS) : % : %.o $(OBJS)
	@echo "LD  $@"
	@$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

#$(MINISAT)/build/release/lib/libminisat.a :
#	cd $(MINISAT); make lr

prof : $(TARGETS)
	rm gmon.out.*
	src/main /tmp/ele4.ll_net

tags : $(SRCS)
	#ctags -R --c++-kinds=+p --fields=+K --extra=+q /usr/include/llvm-3.6/ .
	ctags -R --c++-kinds=+p --fields=+K --extra=+q ./src

g gdb : $(TARGETS)
	gdb ./src/main

vars :
	@echo CC $(CC)
	@echo CXX $(CXX)
	@echo SRCS $(SRCS)
	@echo MSRCS $(MSRCS)
	@echo OBJS $(OBJS)
	@echo MOBJS $(MOBJS)
	@echo TARGETS $(TARGETS)
	@echo DEPS $(DEPS)

clean :
	@rm -f $(TARGETS) $(MOBJS) $(OBJS)
	@rm -f src/cna/spec_lexer.cc src/cna/spec_parser.cc src/cna/spec_parser.h
	@echo Cleaning done.

distclean : clean
	@rm -f $(DEPS)
	@rm -Rf dist/
	@find examples/ -name '*.cnf' -exec rm '{}' ';'
	@find examples/ -name '*.mci' -exec rm '{}' ';'
	@find examples/ -name '*.bc' -exec rm '{}' ';'
	@find examples/ -name '*.r' -exec rm '{}' ';'
	@find examples/ -name '*.cuf' -exec rm '{}' ';'
	@find examples/ -name '*.dot' -exec rm '{}' ';'
	@find examples/ -name '*.pdf' -exec rm '{}' ';'
	@find examples/ -name '*.tr' -exec rm '{}' ';'
	@find examples/ -name '*.pt' -exec rm '{}' ';'
	@#rm -f test/nets/{plain,cont,pr}/{small,med,large,huge}/*.{cnf,mci,bc,r,cuf,dot,pdf}
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

-include $(DEPS)

