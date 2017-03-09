
# Copyright (C) 2010--2017  Cesar Rodriguez <cesar.rodriguez@lipn.fr>
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

# R         Optional (default "."). Path to the root directory of the project.
# D         Optional (default "$R"). Path to the subdir where we are invoking this.
# INC       Optional (default ""). Options -I and -D for the cpp.
# SRCS      Optional (default "$D/*.{c,cc}"). Sources to compile.
# MSRCS     Optional (default "$D/main.cc"). Sources that contain a main()
# LIBS      Optional (default ""). Libraries to link with (.{a,o,so} files)
#
# defines
# CC
# CXX
# CFLAGS, CPPFLAGS, LDFLAGS
# TARGETS
# COMPILE.cc
# LINK.cc
#
# provides goals:
#
# compile
# clean
# realclean
# vars
# c -> [oi]
# cc -> [oi]
# dot -> {svg,pdf}

.DEFAULT_GOAL := all
.PHONY: all clean realclean compile

# default values for the input variables
R ?= .
D ?= $R
SRCS ?= $(wildcard $D/*.c $D/*.cc $D/*/*.c $D/*/*.cc $D/*/*/*.c $D/*/*/*.cc)
MSRCS ?= $(wildcard $D/main.cc) $(wildcard $D/*/main.cc) $(wildcard $D/*/*/main.cc)

# compilation targets
TMP = $(SRCS:.cc=.o)
OBJS ?= $(TMP:.c=.o)
TMP2 = $(MSRCS:.cc=.o)
MOBJS ?= $(TMP2:.c=.o)
TARGETS ?= $(MOBJS:.o=)

include $R/config.mk

# == llvm ==
#LLVMCXXFLAGS := -I$(shell llvm-config-$(CONFIG_LLVM_VER) --includedir)
LLVMCXXFLAGS := $(shell llvm-config-$(CONFIG_LLVM_VER) --cppflags)
LLVMLDFLAGS := $(shell llvm-config-$(CONFIG_LLVM_VER) --ldflags)
#LLVMLIBS := $(shell llvm-config-$(CONFIG_LLVM_VER) --libs --system-libs)
LLVMLIBS := $(shell llvm-config-$(CONFIG_LLVM_VER) --libs all) -lz -lpthread -lffi -lncurses -ldl -lm

# == steroids ==
STIDCPPFLAGS := -I $(CONFIG_STEROIDS_ROOT)/include/
STIDLDFLAGS := -L $(CONFIG_STEROIDS_ROOT)/src/
STIDLDLIBS := -Wl,-Bstatic -lsteroids -Wl,-Bdynamic
#STIDLDLIBS := -lsteroids

# traditional variables
ifdef CONFIG_DEBUG
CFLAGS = -Wall -std=c11 -g
CXXFLAGS = -Wall -std=c++11 -g
endif
ifdef CONFIG_RELEASE
CFLAGS = -Wall -std=c11 -g -O3
CXXFLAGS = -Wall -std=c++11 -O3
endif
CPPFLAGS = -I $D $(LLVMCXXFLAGS) $(STIDCPPFLAGS) $(INC)
LDFLAGS = $(LLVMLDFLAGS) $(STIDLDFLAGS) # -dead_strip -static
LDLIBS = $(LIBS) $(STIDLDLIBS) $(LLVMLIBS)

# define the toolchain
VERS:=-3.7
LD := ld$(VERS)
CC := clang$(VERS)
CXX := clang++$(VERS)
CPP := cpp$(VERS)
LEX := flex
YACC := bison

COMPILE.c = $(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<
COMPILE.cc = $(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<
LINK.c = $(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)
LINK.cc = $(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

%.d : %.c
	@echo "DEP $<"
	@$(CC) -MM -MT "$*.o $*.i $@" $(CFLAGS) $(CPPFLAGS) $< -o $@

%.d : %.cc
	@echo "DEP $<"
	@$(CXX) -MM -MT "$*.o $*.i $@" $(CXXFLAGS) $(CPPFLAGS) $< -o $@

%.cc : %.l
	@echo "LEX $<"
	@$(LEX) -o $@ $^

%.cc : %.y
	@echo "YAC $<"
	@$(YACC) -o $@ $^

#%.h : %.mk
#	@echo "M2H $<"
#	@$R/scripts/mk2h.py < $^ > $@

# cancelling gnu make builtin rules for lex/yacc to c
# http://ftp.gnu.org/old-gnu/Manuals/make-3.79.1/html_chapter/make_10.html#SEC104
%.c : %.l
%.c : %.y

%.o : %.c
	@echo "CC  $<"
	@$(COMPILE.c)

%.o : %.cc
	@echo "CXX $<"
	@$(COMPILE.cc)

%.i : %.c
	@echo "CC  $<"
	@$(COMPILE.c) -E

%.i : %.cc
	@echo "CXX $<"
	@$(COMPILE.cc) -E

%.pdf : %.dot
	@echo "DOT $<"
	@dot -T pdf < $< > $@

%.png : %.dot
	@echo "DOT $<"
	@dot -Tpng < $< > $@

%.svg : %.dot
	@echo "DOT $<"
	@dot -Tsvg < $< > $@

CFLAGS_:=-Wall -Wextra -std=c11 -O3
CXXFLAGS_:=-Wall -Wextra -std=c++11 -O3

%.ll : %.c
	$(CC) $(CFLAGS_) -S -flto $< -o $@
%.bc : %.c
	$(CC) $(CFLAGS_) -c -flto $< -o $@
%.ll : %.cc
	$(CXX) $(CXXFLAGS_) -S -flto $< -o $@
%.bc : %.cc
	$(CXX) $(CXXFLAGS_) -c -flto $< -o $@
%.bc : %.ll
	llvm-as-$(CONFIG_LLVM_VER) $< -o $@
%.ll : %.bc
	llvm-dis-$(CONFIG_LLVM_VER) $< -o $@
%.s : %.bc
	llc-$(CONFIG_LLVM_VER) $< -o $@

compile: $(TARGETS)

$(TARGETS) : % : %.o $(OBJS) $(LIBS)
	@echo "LD  $@"
	@$(LINK.cc)
    
vars :
	@echo "R          $R"
	@echo "D          $D"
	@echo "INC        $(INC)"
	@echo "SRCS       $(SRCS)"
	@echo "MSRCS      $(MSRCS)"
	@echo "LIBS       $(LIBS)"
	@echo "======="
	@echo "CC         $(CC)"
	@echo "CXX        $(CXX)"
	@echo "CFLAGS     $(CFLAGS)"
	@echo "CPPFLAGS   $(CPPFLAGS)"
	@echo "CXXFLAGS   $(CXXFLAGS)"
	@echo "LDFLAGS    $(LDFLAGS)"
	@echo "LDLIBS     $(LDLIBS)"
	@echo "MOBJS      $(MOBJS)"
	@echo "OBJS       $(OBJS)"
	@echo "======="
	@echo "COMPILE.cc $(COMPILE.cc)"
	@echo "LINK.cc    $(LINK.cc)"
	@echo "TARGETS    $(TARGETS)"

clean :
	@rm -f $(TARGETS) $(MOBJS) $(OBJS)

realclean : clean
	@rm -f $(DEPS)

# dependency files
DEPS = $(patsubst %.o,%.d,$(OBJS) $(MOBJS))
$(DEPS) : $R/config.h
-include $(DEPS)

