
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

.PHONY: all g test distclean realclean dist compile tags run run2 dot
.PHONY: unittest regression
	
include config.mk

ifeq ($(shell id -nu),cesar)
all : dist run2
else
all : dist
endif

dist : compile $(CONFIG_STEROIDS_ROOT)/rt/rt.bc
	rm -Rf dist/
	mkdir dist/
	mkdir dist/bin
	mkdir dist/lib
	mkdir dist/lib/dpu
	
	cp src/driver.sh dist/bin/dpu
	cp src/main dist/lib/dpu/dpu-backend
	cp ../steroid/rt/rt.bc dist/lib/dpu/

compile :
	make -f src/Makefile R=. $@

run: dist
	./dist/bin/dpu benchmarks/basic/cjlu.c -vv --dot u.dot -- p main3
	make u.svg

run2: dist
	./dist/bin/dpu benchmarks/basic/cjlu.c -vv --dot u.dot -- p main4
	make u.svg

tags :
	ctags -R --c++-kinds=+p --fields=+K --extra=+q src/ $(shell llvm-config-$(LLVMVERS) --includedir)

g gdb : dist
	./dist/bin/dpu benchmarks/basic/cjlu.c -vv --gdb -- p main3

tests : unittests regression

unittest :
	make -f tests/unit/Makefile R=.
regression :
	make -f tests/regression/Makefile R=.

clean : clean_
	make -f src/Makefile R=. $@
	make -f tests/unit/Makefile R=. $@

distclean : realclean
realclean :
	@make -f src/Makefile R=. $@
	@make -f tests/unit/Makefile R=. $@
	@rm -f $(DEPS)
	@rm -Rf dist/
	@echo Mr. Proper done.

install : dist
	cd dist; cp -Rv * $(CONFIG_PREFIX)

DOTS=$(wildcard dot/*.dot)
PDFS=$(DOTS:.dot=.pdf)
SVGS=$(DOTS:.dot=.svg)

dot: $(SVGS)

o open :
	(ls dot/*.svg | head -n1 | xargs eog) &
 	     
clean : clean_
clean_ :
	@rm -f u.dot
	@rm -f dot/*.dot
	@rm -f dot/*.png
	@rm -f dot/*.pdf

