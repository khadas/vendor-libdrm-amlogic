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

typedef enum {
    MESON_DRM_AR_UNKNOWN      = 0,
    MESON_DRM_AR_4_3         = 1,
    MESON_DRM_AR_16_9 = 2
} ENUM_MESON_DRM_ASPECT_RATIO;

typedef enum _ENUM_MESON_DRM_PROP{
    ENUM_DRM_PROP_HDMI_ENABLE = 0,
    ENUM_DRM_PROP_HDMITX_EOTF,
    ENUM_DRM_PROP_CONTENT_PROTECTION,
    ENUM_DRM_PROP_HDCP_VERSION,
    ENUM_DRM_PROP_HDR_POLICY,
    ENUM_DRM_PROP_GETRX_HDCP_SUPPORTED_VERS,
    ENUM_DRM_PROP_GETRX_HDR_CAP,
    ENUM_DRM_PROP_GETTX_HDR_MODE,
    ENUM_DRM_PROP_GETRX_HDCP_AUTHMODE,
    ENUM_DRM_PROP_HDMI_ASPECT_RATIO,
    ENUM_DRM_PROP_HDMI_DV_ENABLE,
    ENUM_DRM_PROP_MAX
} ENUM_MESON_DRM_PROP;

typedef struct _DisplayMode {
    uint16_t w;
    uint16_t h;
    uint32_t vrefresh;
    bool interlace;
    char name[DRM_DISPLAY_MODE_LEN];
} DisplayMode;

typedef enum _ENUM_MESON_DRM_HDR_MODE {
    MESON_DRM_HDR10PLUS      = 0,
    MESON_DRM_DOLBYVISION_STD,
    MESON_DRM_DOLBYVISION_LL,
    MESON_DRM_HDR10_ST2084,
    MESON_DRM_HDR10_TRADITIONAL,
    MESON_DRM_HDR_HLG,
    MESON_DRM_SDR
} ENUM_MESON_DRM_HDR_MODE;

typedef enum _ENUM_HDCP_VERSION {
    ENUM_HDCP_VERSION_DEFAULT = 0,
    ENUM_HDCP_VERSION_FORCE_2_2,
    ENUM_HDCP_VERSION_FORCE_1_4,
    ENUM_HDCP_VERSION_MAX
} ENUM_HDCP_VERSION;

int meson_drm_setMode(DisplayMode* mode);
int meson_drm_getMode(DisplayMode* mode);

int meson_drm_getRxPreferredMode( DisplayMode* mode);
int meson_drm_getRxSurportedModes( DisplayMode** modes, int* modeCount );
int meson_drm_getEDID( int * data_Len, char **data);

ENUM_MESON_DRM_CONNECTION meson_drm_getConnection();
int meson_drm_set_prop( ENUM_MESON_DRM_PROP enProp, int prop_value );
int meson_drm_get_prop( ENUM_MESON_DRM_PROP enProp, uint32_t* prop_value );
int meson_drm_get_vblank_time(int drmFd, int nextVsync,uint64_t *vblankTime, uint64_t *refreshInterval);
void meson_drm_close_fd(int drmFd);
int meson_drm_open();

#if defined(__cplusplus)
}
#endif

#endif
