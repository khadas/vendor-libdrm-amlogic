LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_LDLIBS += -ldrm -lpthread

LOCAL_SRC_FILES := drm_setcrtc.c

LOCAL_MODULE := drm_setcrtc
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := by_exception_only notice restricted
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES += libdrm
include $(BUILD_EXECUTABLE)