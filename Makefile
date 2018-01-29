
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

dist : compile $(CONFIG_STEROIDS_ROOT)/rt/rt.bc
	rm -Rf dist/
	mkdir dist/
	mkdir dist/bin
	mkdir dist/include
	mkdir dist/lib
	mkdir dist/lib/dpu
	
	cp src/driver.sh dist/bin/dpu
	cp src/main dist/lib/dpu/dpu-backend
	cp rt/verifier.h dist/include
	llvm-link-3.7 $(CONFIG_STEROIDS_ROOT)/rt/rt.bc rt/verifier.bc -o dist/lib/dpu/rt.bc

compile :
	+make -f src/Makefile R=. $@
	+make -f rt/Makefile R=. $@

run: dist
	./dist/bin/dpu benchmarks/basic/cjlu.c -vv --dot u.dot -- p main3
	make u.svg

run2: dist
	./dist/bin/dpu benchmarks/basic/cjlu.c -vv --dot u.dot -- p main4
	make u.svg

tags :
	ctags -R --c++-kinds=+p --fields=+K --extra=+q src/ tests/unit/ config.h $(shell llvm-config$(LLVMVERS) --includedir)

g gdb : dist
	./dist/bin/dpu benchmarks/basic/cjlu.c -vv --gdb -- p main3

c cgdb : dist
	./dist/bin/dpu benchmarks/basic/cjlu.c -vv --gdb -- p main3

tests : unittests regression

unittest ut : compile
	make -f tests/unit/Makefile R=.
	make u.svg

regression : dist
	make -f tests/regression/Makefile R=.

REL:=dpu-$(shell uname -p)-$(CONFIG_VERSION)

release : dist
	rm -Rf $(REL)
	cp -Rv dist $(REL)
	cp LICENSE $(REL)
	cp README.rst $(REL)
	tar czvf $(REL).tar.gz $(REL)

clean : clean_
clean_ :
	make -f src/Makefile R=. clean
	make -f rt/Makefile R=. clean
	make -f tests/unit/Makefile R=. clean
	make -f tests/regression/Makefile R=. clean
	rm -f regression.log*
	rm -f u.dot
	rm -f dot/*.dot
	rm -f dot/*.png
	rm -f dot/*.pdf

distclean : realclean
realclean : realclean_
realclean_ :
	make -f src/Makefile R=. realclean
	make -f tests/unit/Makefile R=. realclean
	make -f tests/regression/Makefile R=. realclean
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

