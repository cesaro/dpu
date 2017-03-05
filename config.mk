
# Root of the DPU project, if undefined
R ?= .

# compilation configuration: debug/release
#CONFIG_RELEASE=debug
CONFIG_RELEASE=release

# folder where the tool will be installed
CONFIG_PREFIX = ~/x/local

# maximum verbosity level (make this conditional)
CONFIG_VERBOSITY_LEVEL = 3

# LLVM version
CONFIG_LLVM_VER := 3.7

# location of the steroids project
CONFIG_STEROIDS_ROOT := $R/../steroid

# configuration parameters
# conditional compilation of features
