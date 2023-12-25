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
meson_drm_SRCS = \
	$(LOCAL_PATH)/libdrm_meson_connector.c \
	$(LOCAL_PATH)/libdrm_meson_property.c \
	$(LOCAL_PATH)/meson_drm.c  \
	$(LOCAL_PATH)/meson_drm_async_commit.c \
	$(LOCAL_PATH)/meson_drm_kms.c  \
	$(LOCAL_PATH)/meson_drm_util.c  \
	$(LOCAL_PATH)/meson_drm_display.c  \
	$(LOCAL_PATH)/meson_drm_event.c  \
	$(LOCAL_PATH)/meson_drm_settings.c  \
	$(LOCAL_PATH)/meson_drm_log.c  \
	$(NULL)

meson_drm_HEADERS = \
	$(LOCAL_PATH) \
	$(NULL)

meson_drm_test_SRCS  = \
	$(LOCAL_PATH)/meson_drm_test.c \
	$(NULL)

meson_drm_display_test_SRCS  = \
	$(LOCAL_PATH)/meson_drm_display_test.c \
	$(NULL)

meson_drm_settings_test_SRCS  = \
	$(LOCAL_PATH)/meson_drm_settings_test.c \
	$(NULL)

OUT_DIR ?= .
# ---------------------------------------------------------------------
#  Build rules
BUILD_TARGETS = libdrm_meson.so meson_drm_test  meson_drm_display_test  meson_drm_settings_test
CLEAN_TARGETS = $(addprefix $(OUT_DIR)/, $(BUILD_TARGETS))

.PHONY: all install uninstall clean

all:$(BUILD_TARGETS)

libdrm_meson.so: $(meson_drm_SRCS)
	$(CC) $(CFLAGS) $(LDFLAGS) -g -D_FILE_OFFSET_BITS=64 -Wall  \
	-shared -fPIC -o $(OUT_DIR)/$@ $^ $(LDLIBS)
	cp -f $(OUT_DIR)/libdrm_meson.so  $(STAGING_DIR)/usr/lib/

meson_drm_test: $(meson_drm_test_SRCS) $(OUT_DIR)/libdrm_meson.so
	$(CC) $(CFLAGS)  -I$(meson_drm_HEADERS) -L$(LOCAL_PATH) \
	-ldrm_meson $(LDFLAGS) -o $(OUT_DIR)/$@ $^ $(LDLIBS)

meson_drm_display_test: $(meson_drm_display_test_SRCS) $(OUT_DIR)/libdrm_meson.so
	$(CC) $(CFLAGS)  -I$(meson_drm_HEADERS) -L$(LOCAL_PATH) \
	-ldrm_meson $(LDFLAGS) -o $(OUT_DIR)/$@ $^ $(LDLIBS)
meson_drm_settings_test: $(meson_drm_settings_test_SRCS) $(OUT_DIR)/libdrm_meson.so
	$(CC) $(CFLAGS)  -I$(meson_drm_HEADERS) -L$(LOCAL_PATH) \
	-ldrm_meson $(LDFLAGS) -o $(OUT_DIR)/$@ $^ $(LDLIBS)

clean:
	rm -f *.o $(CLEAN_TARGETS)

install:
	install -m 0644 $(OUT_DIR)/libdrm_meson.so $(TARGET_DIR)/usr/lib
	install -m 755 $(OUT_DIR)/meson_drm_test $(TARGET_DIR)/usr/bin/
	install -m 755 $(OUT_DIR)/meson_drm_display_test $(TARGET_DIR)/usr/bin/
	install -m 755 $(OUT_DIR)/meson_drm_settings_test $(TARGET_DIR)/usr/bin/
	cp meson_drmif.h $(STAGING_DIR)/usr/include/
	cp meson_drm_util.h $(STAGING_DIR)/usr/include/
	install -D -m 0644 meson_drm_settings.h $(STAGING_DIR)/usr/include/libdrm_meson/meson_drm_settings.h
	install -D -m 0644 meson_drm_event.h $(STAGING_DIR)/usr/include/libdrm_meson/meson_drm_event.h
	install -D -m 0644 meson_drm_log.h $(STAGING_DIR)/usr/include/libdrm_meson/meson_drm_log.h

uninstall:
	rm -f $(TARGET_DIR)/usr/lib/libdrm_meson.so
	rm -f $(TARGET_DIR)/usr/bin/meson_drm_test
	rm -f $(TARGET_DIR)/usr/bin/meson_drm_display_test
	rm -f $(TARGET_DIR)/usr/bin/meson_drm_settings_test

