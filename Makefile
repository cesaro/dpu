
# Copyright (C) 2010-2017  Cesar Rodriguez <cesar.rodriguez@lipn.fr>
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

.PHONY: all g test distclean realclean dist compile tags run run2 dot
.PHONY: unittest regression

R:=.
SRCS:=
MSRCS:=
include common.mk

all : dist

dist : compile $(STIDROOT)/rt/rt.bc
	rm -Rf dist/
	mkdir dist/
	mkdir dist/bin
	mkdir dist/include
	mkdir dist/lib
	mkdir dist/lib/dpu
	
	cp src/driver.sh dist/bin/dpu
	cp src/main dist/lib/dpu/dpu-backend
	cp rt/verifier.h dist/include
	llvm-link-$(CONFIG_LLVM_VER) $(STIDROOT)/rt/rt.bc rt/verifier.bc -o dist/lib/dpu/rt.bc

compile :
	+$(MAKE) -f src/Makefile R=. $@ $(MAKEFLAGS)
	+$(MAKE) -f rt/Makefile R=. $@ $(MAKEFLAGS)

.PHONY: $(STIDROOT)/rt/rt.bc
$(STIDROOT)/rt/rt.bc :
	$(MAKE) -C lib/steroids rt/rt.bc

run: dist
	./dist/bin/dpu benchmarks/basic/cjlu.c -vv --dot u.dot -- p main3
	$(MAKE) u.svg

run2: dist
	./dist/bin/dpu benchmarks/basic/cjlu.c -vv --dot u.dot -- p main4
	$(MAKE) u.svg

tags :
	ctags -R --c++-kinds=+p --fields=+K --extra=+q src/ tests/unit/ config.h $(shell llvm-config-$(CONFIG_LLVM_VER) --includedir)

g gdb : dist
	./dist/bin/dpu benchmarks/basic/cjlu.c -vv --gdb -- p main3

c cgdb : dist
	./dist/bin/dpu benchmarks/basic/cjlu.c -vv --gdb -- p main3

tests : unittests regression

unittest ut : compile
	$(MAKE) -f tests/unit/Makefile R=. $(MAKEFLAGS)
	$(MAKE) u.svg

regression : dist
	$(MAKE) -f tests/regression/Makefile R=. $(MAKEFLAGS)

REL:=dpu-$(shell uname -p)-$(CONFIG_VERSION)

release : dist
	rm -Rf $(REL)
	cp -Rv dist $(REL)
	cp README.rst $(REL)
	tar czvf $(REL).tar.gz $(REL)

cav18-release : release
	git archive HEAD experiments src rt | tar x -C $(REL)
	rm -R $(REL)/experiments/stuff
	mv $(REL)/experiments/cav18/README.CAV18.rst $(REL)
	./scripts/anonymize.sh $(REL)

clean : clean_
clean_ :
	$(MAKE) -f src/Makefile R=. clean $(MAKEFLAGS)
	$(MAKE) -f rt/Makefile R=. clean $(MAKEFLAGS)
	$(MAKE) -f tests/unit/Makefile R=. clean $(MAKEFLAGS)
	$(MAKE) -f tests/regression/Makefile R=. clean $(MAKEFLAGS)
	rm -f u.dot
	rm -f dot/*.dot
	rm -f dot/*.png
	rm -f dot/*.pdf

distclean : realclean
realclean : realclean_
realclean_ :
	$(MAKE) -f src/Makefile R=. realclean $(MAKEFLAGS)
	$(MAKE) -f tests/unit/Makefile R=. realclean $(MAKEFLAGS)
	$(MAKE) -f tests/regression/Makefile R=. realclean $(MAKEFLAGS)
	rm -Rf dist/ $(REL)
	rm -f config.h
	rm -f regression.log*
	rm -f $(REL).tar.gz

install : dist
	cd dist; cp -Rv * $(CONFIG_PREFIX)

DOTS=$(wildcard dot/*.dot)
PDFS=$(DOTS:.dot=.pdf)
SVGS=$(DOTS:.dot=.svg)

dot: $(SVGS)

o open :
	(ls dot/*.svg | head -n1 | xargs eog) &

