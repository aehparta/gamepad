
# check that LIBE_PATH is set
ifeq ($(LIBE_PATH),)
    $(error LIBE_PATH not set)
endif

# detect target and do other init stuff
include $(LIBE_PATH)/target.mk
include $(LIBE_PATH)/source.mk
include $(LIBE_PATH)/flags.mk

# source files
daemon_SRC = main.c gdd.c cmd.c $(libe_SRC)

# build binaries
# BUILD_BINS = daemon
BUILD_LIBS = libe

# compile flags
CFLAGS += -D_GNU_SOURCE $(libe_CFLAGS)
LDFLAGS += $(libe_LDFLAGS)

include $(LIBE_PATH)/cc.mk
$(info libe sources: $(libe_SRC))
$(info libe use: $(libe_USE))
$(info cflags: $(CFLAGS))
$(info ldflags: $(LDFLAGS))
$(info bins: $(BUILD_BINS))
$(info libs: $(BUILD_LIBS))
$(info $(CC_PREFIX))
$(info )
include $(LIBE_PATH)/build.mk
