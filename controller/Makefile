
# check that LIBE_PATH is set
ifeq ($(LIBE_PATH),)
    $(error LIBE_PATH not set)
endif

libe_USE += SPI

# init
include $(LIBE_PATH)/init.mk

# our own sources etc
BUILD_BINS = controller
controller_SRC = main.c $(libe_SRC)

# compile flags
CFLAGS += $(libe_CFLAGS)
LDFLAGS += $(libe_LDFLAGS)

# build
include $(LIBE_PATH)/build.mk
