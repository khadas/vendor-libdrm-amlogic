/*
 * Copyright (c) 2021 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#ifndef MESON_DRM_SETTINGS_H_
#define MESON_DRM_SETTINGS_H_
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

typedef enum _MESON_CONTENT_TYPE {
    MESON_CONTENT_TYPE_Data      = 0,
    MESON_CONTENT_TYPE_Graphics,
    MESON_CONTENT_TYPE_Photo,
    MESON_CONTENT_TYPE_Cinema,
    MESON_CONTENT_TYPE_Game,
    MESON_CONTENT_TYPE_RESERVED
} MESON_CONTENT_TYPE;

/*HDCP transmission time divided into Type0&Type1 content*/
typedef enum _ENUM_MESON_HDCP_Content_Type{
    MESON_HDCP_Type0 = 0,  //Type0 represents support for both 1.4 and 2.2
    MESON_HDCP_Type1,   //Type1 represents only support for 2.2
    MESON_HDCP_Type_RESERVED
} ENUM_MESON_HDCP_Content_Type;

typedef enum {
    MESON_DISCONNECTED      = 0,
    MESON_CONNECTED         = 1,
    MESON_UNKNOWNCONNECTION = 2
} ENUM_MESON_CONN_CONNECTION;

typedef struct _DisplayMode {
    uint16_t w;  //<--Number of horizontal pixels in the effective display area-->//
    uint16_t h;   //<--Number of vertical pixels in the effective display area-->//
    uint32_t vrefresh;  //<--Display refresh rate--->//
    bool interlace;  //<--Indicates which scanning form to choose, P represents progressive scanning, and i represents interlaced scanning; The default interlace value is 0 for P 1 for i-->//
    char name[DRM_DISPLAY_MODE_LEN];
} DisplayMode;

typedef enum _ENUM_MESON_HDR_MODE {
    MESON_HDR10PLUS      = 0,
    MESON_DOLBYVISION_STD,
    MESON_DOLBYVISION_LL,
    MESON_HDR10_ST2084,
    MESON_HDR10_TRADITIONAL,
    MESON_HDR_HLG,
    MESON_SDR
} ENUM_MESON_HDR_MODE;

typedef enum _ENUM_MESON_CONNECTOR_TYPE {
    MESON_CONNECTOR_HDMIA      = 0,
    MESON_CONNECTOR_HDMIB,
    MESON_CONNECTOR_LVDS,
    MESON_CONNECTOR_CVBS,
    MESON_CONNECTOR_RESERVED
} MESON_CONNECTOR_TYPE;

typedef enum _ENUM_MESON_COLOR_SPACE {
    MESON_COLOR_SPACE_RGB      = 0,
    MESON_COLOR_SPACE_YCBCR422,
    MESON_COLOR_SPACE_YCBCR444,
    MESON_COLOR_SPACE_YCBCR420,
    MESON_COLOR_SPACE_RESERVED
} ENUM_MESON_COLOR_SPACE;

typedef enum _ENUM_MESON_HDCP_VERSION {
    MESON_HDCP_14      = 0,
    MESON_HDCP_22,
    MESON_HDCP_RESERVED
} ENUM_MESON_HDCP_VERSION;

typedef enum _ENUM_MESON_HDR_POLICY {
    MESON_HDR_POLICY_FOLLOW_SINK      = 0,
    MESON_HDR_POLICY_FOLLOW_SOURCE
} ENUM_MESON_HDR_POLICY;

typedef enum _ENUM_MESON_HDCP_AUTH_STATUS {
    MESON_AUTH_STATUS_FAIL      = 0,
    MESON_AUTH_STATUS_SUCCESS
} ENUM_MESON_HDCPAUTH_STATUS;

int meson_drm_setContentType(int drmFd, drmModeAtomicReq *req,
                       MESON_CONTENT_TYPE contentType, MESON_CONNECTOR_TYPE connType);

int meson_drm_setVrrEnabled(int drmFd, drmModeAtomicReq *req,
                       uint32_t VrrEnable, MESON_CONNECTOR_TYPE connType);
int meson_drm_getVrrEnabled( int drmFd, MESON_CONNECTOR_TYPE connType );


int meson_drm_getActive( int drmFd, MESON_CONNECTOR_TYPE connType );

int meson_drm_setActive(int drmFd, drmModeAtomicReq *req,
                       uint32_t active, MESON_CONNECTOR_TYPE connType);

int meson_drm_getDvEnable( int drmFd, MESON_CONNECTOR_TYPE connType );
int meson_drm_setDvEnable(int drmFd, drmModeAtomicReq *req,
                       uint32_t dvEnable, MESON_CONNECTOR_TYPE connType);

MESON_CONTENT_TYPE meson_drm_getContentType(int drmFd, MESON_CONNECTOR_TYPE connType );


int meson_drm_changeMode(int drmFd, drmModeAtomicReq *req,
                      DisplayMode* modeInfo, MESON_CONNECTOR_TYPE connType);
int meson_drm_getModeInfo(int drmFd, MESON_CONNECTOR_TYPE connType, DisplayMode* mode );

ENUM_MESON_CONN_CONNECTION meson_drm_getConnectionStatus(int drmFd, MESON_CONNECTOR_TYPE connType);
ENUM_MESON_HDR_MODE meson_drm_getHdrStatus(int drmFd, MESON_CONNECTOR_TYPE connType );
ENUM_MESON_HDCP_VERSION meson_drm_getHdcpVersion(int drmFd, MESON_CONNECTOR_TYPE connType );
ENUM_MESON_COLOR_SPACE meson_drm_getColorSpace(int drmFd, MESON_CONNECTOR_TYPE connType );
int meson_drm_setColorSpace(int drmFd, drmModeAtomicReq *req,
                       ENUM_MESON_COLOR_SPACE colorSpace, MESON_CONNECTOR_TYPE connType);

uint32_t meson_drm_getColorDepth(int drmFd, MESON_CONNECTOR_TYPE connType );
int meson_drm_setColorDepth(int drmFd, drmModeAtomicReq *req,
                       uint32_t colorDepth, MESON_CONNECTOR_TYPE connType);

ENUM_MESON_HDR_POLICY meson_drm_getHDRPolicy(int drmFd, MESON_CONNECTOR_TYPE connType );
int meson_drm_setHDRPolicy(int drmFd, drmModeAtomicReq *req,
                           ENUM_MESON_HDR_POLICY hdrPolicy, MESON_CONNECTOR_TYPE connType);


void meson_drm_getEDIDData(int drmFd, MESON_CONNECTOR_TYPE connType, int * data_Len, char **data );
int meson_drm_setAVMute(int drmFd, drmModeAtomicReq *req,
                       bool mute, MESON_CONNECTOR_TYPE connType);
int meson_drm_getAVMute( int drmFd, MESON_CONNECTOR_TYPE connType );

ENUM_MESON_HDCPAUTH_STATUS meson_drm_getHdcpAuthStatus( int drmFd, MESON_CONNECTOR_TYPE connType );
int meson_drm_setHDCPEnable(int drmFd, drmModeAtomicReq *req,
                       bool enable, MESON_CONNECTOR_TYPE connType);

int meson_drm_getsupportedModesList(int drmFd, DisplayMode** modeInfo, int* modeCount ,MESON_CONNECTOR_TYPE connType);
int meson_drm_getPreferredMode( DisplayMode* mode, MESON_CONNECTOR_TYPE connType);

int meson_drm_setHDCPContentType(int drmFd, drmModeAtomicReq *req,
                       ENUM_MESON_HDCP_Content_Type HDCPType, MESON_CONNECTOR_TYPE connType);
ENUM_MESON_HDCP_Content_Type meson_drm_getHDCPContentType( int drmFd, MESON_CONNECTOR_TYPE connType );

int meson_drm_getHdcpVer( int drmFd, MESON_CONNECTOR_TYPE connType );

int meson_drm_getHdrCap( int drmFd, MESON_CONNECTOR_TYPE connType );
int meson_drm_getDvCap( int drmFd, MESON_CONNECTOR_TYPE connType );

int meson_open_drm();
void meson_close_drm(int drmFd);

#if defined(__cplusplus)
}
#endif

#endif
