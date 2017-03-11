
# Root of the DPU project, if undefined
R ?= .

# compilation configuration: debug/release
CONFIG_DEBUG = 1
#CONFIG_RELEASE = 1

# version of the tool
CONFIG_VERSION = v0.2.0

# folder where the tool will be installed
CONFIG_PREFIX = ~/x/local

# maximum verbosity level at which the tool produces output when requested to be
# verbose with --verb=N
ifdef CONFIG_DEBUG
CONFIG_MAX_VERB_LEVEL = 3
endif
ifdef CONFIG_RELEASE
CONFIG_MAX_VERB_LEVEL = 2
endif

# LLVM version
CONFIG_LLVM_VER = 3.7

# location of the steroids project
CONFIG_STEROIDS_ROOT = $R/../steroid

# maximum number of unfolding processes
CONFIG_MAX_PROCESSES = 50

# maximum number of events per process
CONFIG_MAX_EVENTS_PER_PROCCESS = 40000

# default memory size for guest code execution
CONFIG_GUEST_DEFAULT_MEMORY_SIZE = $(shell echo '128 * 2^20' | bc)

# default size of the stack per thread
CONFIG_GUEST_DEFAULT_THREAD_STACK_SIZE = $(shell echo '4 * 2^20' | bc)

# number of events in the communication buffer between steroids and DPU
CONFIG_GUEST_TRACE_BUFFER_SIZE = $(shell echo '2^20' | bc)

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
