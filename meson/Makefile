OBJ = meson_drm.c meson_drm_kms.c meson_drm_util.c meson_drm_async_commit.c

TARGET = libdrm_meson.so

all: $(TARGET)

TARGET_CFLAGS += -I$(STAGING_DIR)/usr/include/ -I$(STAGING_DIR)/usr/include/libdrm/
TARGET_LDFLAGS += -L$(STAGING_DIR)/usr/lib -ldrm

$(TARGET): $(OBJ)
	$(CC) $(TARGET_CFLAGS) $(TARGET_LDFLAGS) -g -D_FILE_OFFSET_BITS=64 -Wall  $(OBJ) -shared -fPIC -o $@

.PHONY: clean

clean:
	rm -f *.o $(TARGET)

install:
	cp meson_drmif.h $(STAGING_DIR)/usr/include/
	cp meson_drm_util.h $(STAGING_DIR)/usr/include/
	cp $(TARGET) $(STAGING_DIR)/usr/lib/
	cp $(TARGET) $(TARGET_DIR)/usr/lib/
