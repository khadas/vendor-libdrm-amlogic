 /*
 * Copyright (c) 2021 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#ifndef LIBDRM_MESON_CONNECTOR_H_
#define LIBDRM_MESON_CONNECTOR_H_
#if defined(__cplusplus)
extern "C" {
#endif
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <xf86drmMode.h>
#define MESON_DRM_HDMITX_PROP_AVMUTE  "MESON_DRM_HDMITX_PROP_AVMUTE"
#define MESON_DRM_HDMITX_PROP_EOTF  "EOTF"
#define DRM_CONNECTOR_PROP_CONTENT_PROTECTION "Content Protection"
#define DRM_CONNECTOR_PROP_CONTENT_TYPE       "HDCP Content Type"
#define DRM_CONNECTOR_PROP_RX_HDCP_SUPPORTED_VER       "hdcp_ver"
#define DRM_CONNECTOR_PROP_RX_HDR_CAP       "hdr_cap"
#define DRM_CONNECTOR_PROP_RX_DV_CAP       "dv_cap"
#define DRM_CONNECTOR_PROP_TX_HDR_MODE       "hdmi_hdr_status"
#define DRM_CONNECTOR_PROP_TX_HDCP_AUTH_MODE       "hdcp_mode"
#define DRM_CONNECTOR_PROP_TX_HDR_POLICY       "meson.crtc.hdr_policy"
#define DRM_CONNECTOR_PROP_TX_ASPECT_RATIO        "aspect ratio"
#define DRM_CONNECTOR_PROP_GETTX_AR_VALUE       "aspect_ratio_value"
#define DRM_CONNECTOR_PROP_HDCP_PRIORITY       "HDCP_CONTENT_TYPE0_PRIORITY"
#define DRM_CONNECTOR_PROP_DV_ENABLE       "dv_enable"
#define DRM_CONNECTOR_PROP_COLOR_SPACE       "color_space"
#define DRM_CONNECTOR_PROP_COLOR_DEPTH       "color_depth"
#define DRM_CONNECTOR_PROP_HDMI_CONTENT_TYPE       "content type"
#define DRM_CONNECTOR_PROP_ACTIVE       "ACTIVE"
#define DRM_CONNECTOR_VRR_ENABLED       "VRR_ENABLED"
#define DRM_CONNECTOR_FRAC_RATE_POLICY      "FRAC_RATE_POLICY"
#define DRM_CONNECTOR_PROP_TX_HDR_OFF      "force_output"
#define DRM_CONNECTOR_DV_MODE       "dv_mode"
#define DRM_CONNECTOR_PROP_DPMS       "DPMS"
#define DRM_CONNECTOR_PROP_BACKGROUND_COLOR     "BACKGROUND_COLOR"

struct mesonPrimaryPlane;
struct mesonConnector;
struct mesonPrimaryPlane* mesonPrimaryPlaneCreate(int drmFd);
struct mesonConnector *mesonConnectorCreate(int drmFd, int type);
int mesonConnectorUpdate(int drmFd, struct mesonConnector *connector);
struct mesonConnector *mesonConnectorDestroy(int drmFd, struct mesonConnector* connector);
void mesonPrimaryPlaneDestroy(int drmFd, struct mesonPrimaryPlane *primaryplane);
int mesonPrimaryPlaneGetFbSize(struct mesonPrimaryPlane* planesize, int* width, int* height);
int mesonConnectorGetPhysicalSize(struct mesonConnector * connector, int* width, int* height);

int mesonConnectorGetId(struct mesonConnector* connector);
/*HDMI capabilities - Rx supported video formats*/
int mesonConnectorGetModes(struct mesonConnector * connector,int drmFd, drmModeModeInfo** modes, int *count_modes);
/*Edid Information:*/
int mesonConnectorGetEdidBlob(struct mesonConnector* connector, int * data_Len, char** data);
/*get connector connect status.*/
int mesonConnectorGetConnectState(struct mesonConnector* connector);
int mesonConnectorGetCRTCId(struct mesonConnector* connector);
drmModeModeInfo* mesonConnectorGetCurMode(int drmFd, struct mesonConnector* connector);
#if defined(__cplusplus)
}
#endif

#endif
