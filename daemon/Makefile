
# check that LIBE_PATH is set
ifeq ($(LIBE_PATH),)
    $(error LIBE_PATH not set)
endif

# init
include $(LIBE_PATH)/init.mk

# our own sources etc
BUILD_BINS = gamepadd
gamepadd_SRC = main.c gdd.c cmd.c $(libe_SRC)

# compile flags
CFLAGS += -D_GNU_SOURCE $(libe_CFLAGS)
LDFLAGS += $(libe_LDFLAGS)

# build
include $(LIBE_PATH)/build.mk
