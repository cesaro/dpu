
R ?= ..
D := $R/src
LIBS = $(STIDROOT)/src/libsteroids.a

include $R/common.mk

.PHONY: $(STIDROOT)/src/libsteroids.a
$(STIDROOT)/src/libsteroids.a :
	$(MAKE) -C $(STIDROOT) src/libsteroids.a

all : compile

