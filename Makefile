
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

.PHONY: fake all g test clean distclean prof dist compile tags run run2 dot
	

ifeq ($(shell id -nu),cesar)
all : dist run2
else
all : compile
endif

compile: $(TARGETS)

run: dist
	rm -f dot/*.dot dot/*.svg
	./dist/bin/dpu benchmarks/basic/cjlu.c -vv --dot dot/u.dot -- p main3
	make dot

run2: dist
	rm -f dot/*.dot dot/*.svg
	./dist/bin/dpu benchmarks/basic/cjlu.c -vv --dot dot/u.dot -- p main4
	make dot

$(TARGETS) : % : %.o $(OBJS)
	@echo "LD  $@"
	@$(CXX) $(LDFLAGS) -o $@ $^ ../steroid/src/libsteroids.a $(LDLIBS)
    
tags : $(SRCS)
	ctags -R --c++-kinds=+p --fields=+K --extra=+q src/ $(shell llvm-config-$(LLVMVERS) --includedir)

g gdb : dist
	./dist/bin/dpu benchmarks/basic/cjlu.c -vv --gdb -- p main3
		
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

dist : compile ../steroid/rt/rt.bc
	rm -Rf dist/
	mkdir dist/
	mkdir dist/bin
	mkdir dist/lib
	mkdir dist/lib/dpu
	
	cp src/driver.sh dist/bin/dpu
	cp src/main dist/lib/dpu/dpu-backend
	cp ../steroid/rt/rt.bc dist/lib/dpu/


PREFIX = ~/x/local

install : dist
	cd dist; cp -Rv * $(PREFIX)

dot: $(SVGS)

o open :
	(ls dot/*.svg | head -n1 | xargs eog) &
 	     
-include $(DEPS)

