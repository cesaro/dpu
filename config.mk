
# Root of the DPU project, if undefined
R ?= .

# compilation configuration: debug/release
#CONFIG_DEBUG = 1
CONFIG_RELEASE = 1

# version of the tool
CONFIG_VERSION = v0.3.0

# folder where the tool will be installed with "make install"
CONFIG_PREFIX = ~/x/local

# LLVM version
CONFIG_LLVM_VER = 3.7

# location of the steroids project
CONFIG_STEROIDS_ROOT = $R/../steroids

# maximum number of unfolding processes
CONFIG_MAX_PROCESSES = 32

# maximum number of events per process
CONFIG_MAX_EVENTS_PER_PROCCESS = 2000100

# default memory size for guest code execution
CONFIG_GUEST_DEFAULT_MEMORY_SIZE = $(shell echo '128 * 2^20' | bc)

# default size of the stack per thread
CONFIG_GUEST_DEFAULT_THREAD_STACK_SIZE = $(shell echo '1 * 2^20' | bc)

# number of events in the communication buffer between steroids and DPU
CONFIG_GUEST_TRACE_BUFFER_SIZE = $(shell echo '1 * 2^20' | bc)

# defines the skip step for the sequential tree skip lists (>= 2)
CONFIG_SKIP_STEP = 2

# defines the maximum number of times that a defect will get repeated in the
# defect report
CONFIG_MAX_DEFECT_REPETITION = 5

# define this to compile support to visualize statistics about causality/conflict
# checks
#CONFIG_STATS_DETAILED =

# maximum verbosity level at which the tool produces output when requested to be
# verbose with --verb=N; touch these lines only if you know what you are doing
ifdef CONFIG_DEBUG
CONFIG_MAX_VERB_LEVEL = 3
endif
ifdef CONFIG_RELEASE
CONFIG_MAX_VERB_LEVEL = 2
endif

########################

CONFIG_CFLAGS=$(CFLAGS)
CONFIG_COMPILE=$(COMPILE.cc)
CONFIG_LINK=$(LINK.cc)
CONFIG_BUILD_DATE=$(shell date -R)
CONFIG_BUILD_COMMIT="$(shell git show --oneline | head -n1 | awk '{print $$1}')"
CONFIG_BUILD_DIRTY=$(shell if git diff-index --quiet HEAD --; then echo 0; else echo 1; fi)

ifdef CONFIG_DEBUG
ifdef CONFIG_RELEASE
$(error CONFIG_DEBUG and CONFIG_RELEASE are both defined, but are mutually exclusive)
endif
endif

CONFIGVARS=$(filter CONFIG_%,$(.VARIABLES))
export $(CONFIGVARS)

$R/config.h : $R/config.mk
	@echo "GEN $@"
	@$R/scripts/env2h.py $(CONFIGVARS) > $@
