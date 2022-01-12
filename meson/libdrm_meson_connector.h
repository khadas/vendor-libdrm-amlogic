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
struct mesonConnector;
struct mesonConnector *mesonConnectorCreate(int drmFd, int type);
int mesonConnectorUpdate(int drmFd, struct mesonConnector *connector);
struct mesonConnector *mesonConnectorDestroy(int drmFd, struct mesonConnector* connector);
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
