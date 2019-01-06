
LIB_PATH ?= ../../lib

# detect target and do other init stuff
include $(LIB_PATH)/init.mk

# source files
daemon_SRC = main.c gdd.c

# build these binaries
BUILD_BIN = daemon
PROGRAM_BIN = daemon

# do actual build
-include local.mk
include $(LIB_PATH)/common.mk
