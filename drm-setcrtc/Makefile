LOCAL_PATH = $(shell pwd)

MIN_GCC_VERSION = "10"
GCC_VERSION := "`$(CC) -dumpversion | cut -f1 -d'.'`"
IS_GCC_ABOVE_MIN_VERSION := $(shell expr "$(GCC_VERSION)" ">=" "$(MIN_GCC_VERSION)")
ifeq "$(IS_GCC_ABOVE_MIN_VERSION)" "1"
CFLAGS += -std=gnu++20
endif

LDFLAGS += -Wl,--no-as-needed -lstdc++ -lpthread  -ldl -lrt -ludev -L$(STAGING_DIR)/usr/lib
CFLAGS += -Wall -Wno-unknown-pragmas -Wno-format -Wno-format-security -Wno-error=unused-result \
          -O3 -fexceptions -fnon-call-exceptions -D_GNU_SOURCE -I$(STAGING_DIR)/usr/include
CFLAGS += -I$(STAGING_DIR)/usr/include/ -I$(STAGING_DIR)/usr/include/libdrm/
LDFLAGS += -L$(STAGING_DIR)/usr/lib -ldrm

drm_setcrtc_SRCS  = \
	$(LOCAL_PATH)/drm_setcrtc.c  \
	$(NULL)
# ---------------------------------------------------------------------
#  Build rules
BUILD_TARGETS = drm_setcrtc

.PHONY: all install uninstall clean

drm_setcrtc: $(drm_setcrtc_SRCS)
	$(CC) $(CFLAGS) -L$(LOCAL_PATH) $(LDFLAGS) -o $@ $^ $(LDLIBS)

all: $(BUILD_TARGETS)

clean:
	rm -f *.o $(BUILD_TARGETS)

install:
	install -m 755 drm_setcrtc $(TARGET_DIR)/usr/bin/

uninstall:
	rm -f $(TARGET_DIR)/usr/bin/drm_setcrtc

