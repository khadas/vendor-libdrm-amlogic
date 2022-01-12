 /*
 * Copyright (c) 2021 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef MESON_DRM_DISPLAY_H_
#define MESON_DRM_DISPLAY_H_
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <xf86drmMode.h>
#if defined(__cplusplus)
extern "C" {
#endif

#define DRM_DISPLAY_MODE_LEN 32
typedef enum
{
	HDMI_EOTF_TRADITIONAL_GAMMA_SDR = 0,
	HDMI_EOTF_SMPTE_ST2084 = 2,
	HDMI_EOTF_MESON_DOLBYVISION = 18,
	HDMI_EOTF_MESON_DOLBYVISION_L = 19,
	HDMI_EOTF_MAX

} ENUM_DRM_HDMITX_PROP_EOTF;

typedef enum {
	MESON_DRM_DISCONNECTED      = 0,
	MESON_DRM_CONNECTED         = 1,
	MESON_DRM_UNKNOWNCONNECTION = 2
} ENUM_MESON_DRM_CONNECTION;

bool meson_drm_init();
void meson_drm_deinit();
int meson_drm_setHDMIEnable(bool enable);
bool meson_drm_getHDMIEnable();
int meson_drm_setVideoFormat(char* videoFormat);
char* meson_drm_getVideoFormat();
int meson_drm_setHDMItxEOTF(ENUM_DRM_HDMITX_PROP_EOTF value);
ENUM_DRM_HDMITX_PROP_EOTF meson_drm_getHDMItxEOTF();
int meson_drm_getRxSurportedModes(drmModeModeInfo **modes);
int meson_drm_getRxPreferreddMode(drmModeModeInfo *mode);
int meson_drm_getRxSurportedEOTF(ENUM_DRM_HDMITX_PROP_EOTF* EOTFs);
ENUM_MESON_DRM_CONNECTION meson_drm_getConnection();
int meson_drm_setContentProtection(bool enable);
int meson_drm_setHDCPVersion(int version);
uint64_t meson_drm_getContentProtection();
uint64_t meson_drm_getHDCPVersion();
int meson_drm_getRxSupportedHDCPVersion(int* count, int** values);
#if defined(__cplusplus)
}
#endif

#endif
