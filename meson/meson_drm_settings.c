/*
 * Copyright (c) 2023 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <linux/string.h>
#include "libdrm_meson_connector.h"
#include "libdrm_meson_property.h"
#include "meson_drm_settings.h"


#define DEFAULT_CARD "/dev/dri/card0"
#define PROP_NAME_MAX_LEN 50
static int meson_drm_get_crtc_prop_value( int drmFd, MESON_CONNECTOR_TYPE connType,
        char* name, uint32_t* prop_value );
static int meson_drm_get_conn_prop_value( int drmFd, MESON_CONNECTOR_TYPE connType,
                                          char* name, uint32_t* propValue );
static int meson_drm_get_prop_value(int drmFd, MESON_CONNECTOR_TYPE connType,
                                     uint32_t objType, char* name, uint32_t* propValue );

static struct mesonConnector* get_current_connector(int drmFd, MESON_CONNECTOR_TYPE connType);
static int meson_drm_set_property(int drmFd, drmModeAtomicReq *req, uint32_t objId,
                                  uint32_t objType, char* name, uint64_t value);

static int meson_drm_get_prop_value(int drmFd, MESON_CONNECTOR_TYPE connType, uint32_t objType, char* name, uint32_t* propValue )
{
    int ret = -1;
    int objID = -1;
    struct mesonConnector* conn = NULL;
    if ( drmFd < 0 || name == NULL || propValue == NULL)
    {
        fprintf(stderr, "\n%s %d drmfd invalid, or property name invalid\n",__FUNCTION__,__LINE__);
        goto out;
    }
    conn = get_current_connector(drmFd, connType);
    if ( conn == NULL )
    {
        fprintf(stderr, "\n%s %d get_current_connector fail\n",__FUNCTION__,__LINE__);
        goto out;
    }
    objID =  mesonConnectorGetId(conn);
    if (objType == DRM_MODE_OBJECT_CRTC)
        objID =  mesonConnectorGetCRTCId(conn);
    struct mesonProperty* meson_prop = NULL;
    meson_prop = mesonPropertyCreate(drmFd, objID, objType, name);
    if (!meson_prop) {
        printf("\n meson_prop create fail\n");
        goto out;
    }
    uint64_t value = mesonPropertyGetValue(meson_prop);
    *propValue = (uint32_t)value;
    fprintf(stderr, "\n prop value:%llu objID:%d,name:%s\n",value, objID,name);
    mesonPropertyDestroy(meson_prop);
    ret = 0;
out:
    if (conn)
        mesonConnectorDestroy(drmFd,conn);
    return ret;
}

static int meson_drm_get_conn_prop_value( int drmFd, MESON_CONNECTOR_TYPE conn_type, char* name, uint32_t* prop_value )
{
    return meson_drm_get_prop_value( drmFd, conn_type, DRM_MODE_OBJECT_CONNECTOR, name, prop_value );
}

static int meson_drm_get_crtc_prop_value( int drmFd, MESON_CONNECTOR_TYPE conn_type, char* name, uint32_t* prop_value )
{
    return meson_drm_get_prop_value( drmFd, conn_type, DRM_MODE_OBJECT_CRTC, name, prop_value );
}

static struct mesonConnector* get_current_connector(int drmFd, MESON_CONNECTOR_TYPE connType)
{
    struct mesonConnector* connector = NULL;
    int drmConnType = DRM_MODE_CONNECTOR_HDMIA;
    if (drmFd < 0) {
        fprintf(stderr, "\n %s %d invalid drmFd return\n",__FUNCTION__,__LINE__);
        return NULL;
    }
    switch (connType)
    {
        case MESON_CONNECTOR_HDMIA:
            drmConnType = DRM_MODE_CONNECTOR_HDMIA;
            break;
        case MESON_CONNECTOR_HDMIB:
            drmConnType = DRM_MODE_CONNECTOR_HDMIB;
            break;
        case MESON_CONNECTOR_LVDS:
            drmConnType = DRM_MODE_CONNECTOR_LVDS;
            break;
        case MESON_CONNECTOR_CVBS:
            drmConnType = DRM_MODE_CONNECTOR_TV;
        default :
            drmConnType = DRM_MODE_CONNECTOR_HDMIA;
            break;
    }
    connector = mesonConnectorCreate(drmFd, drmConnType);
    return connector;
}
int meson_open_drm()
{
    int ret_fd = -1;
    const char *card;
    int ret = -1;
    card= getenv("WESTEROS_DRM_CARD");
    if ( !card ) {
        card = DEFAULT_CARD;
    }
    ret_fd = open(card, O_RDONLY|O_CLOEXEC);
    if ( ret_fd < 0 )
        fprintf(stderr, "\n meson_open_drm  drm card:%s open fail\n",card);
    else
        drmDropMaster(ret_fd);
    ret = drmSetClientCap(ret_fd, DRM_CLIENT_CAP_ATOMIC, 1);
    if (ret < 0)
        fprintf(stderr, "Unable to set DRM atomic capability\n");

    ret = drmSetClientCap(ret_fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);
    if (ret < 0)
        fprintf(stderr, "Unable to set UNIVERSAL_PLANES\n");
    return ret_fd;
}
void meson_close_drm(int drmFd)
{
    if (drmFd >= 0)
        close(drmFd);
}
static int meson_drm_set_property(int drmFd, drmModeAtomicReq *req, uint32_t objId,
                                  uint32_t objType, char* name, uint64_t value)
{
    uint32_t propId;
    int rc = -1;
    if (drmFd < 0 || req == NULL) {
        fprintf(stderr, "\n %s %d invalid parameter return\n",__FUNCTION__,__LINE__);
        return rc;
    }
    struct mesonProperty *prop = NULL;
    prop = mesonPropertyCreate(drmFd, objId, objType, name);
    propId = mesonPropertyGetId(prop);
    mesonPropertyDestroy(prop);
    fprintf(stderr, "\nmeson_drm_set_property name:%s propId:%d value:%llu\n", name, propId, value);
    rc = drmModeAtomicAddProperty( req, objId, propId, value );
    if (rc < 0)
        fprintf(stderr, "\n %s %d meson_drm_set_property fail\n",__FUNCTION__,__LINE__);
    return rc;
}

int meson_drm_getModeInfo(int drmFd, MESON_CONNECTOR_TYPE connType, DisplayMode* modeInfo)
{
    int ret = -1;
    struct mesonConnector* conn = NULL;
    drmModeModeInfo* mode = NULL;
    if (modeInfo == NULL || drmFd < 0) {
        fprintf(stderr, "\n %s %d modeInfo == NULL || drmFd < 0 return\n",__FUNCTION__,__LINE__);
        return ret;
    }
    conn = get_current_connector(drmFd, connType);
    if ( conn ) {
        mode = mesonConnectorGetCurMode(drmFd, conn);
        if (mode) {
            modeInfo->w = mode->hdisplay;
            modeInfo->h = mode->vdisplay;
            modeInfo->vrefresh = mode->vrefresh;
            modeInfo->interlace = mode->flags & DRM_MODE_FLAG_INTERLACE;
            strcpy(modeInfo->name, mode->name);
            free(mode);
            mode = NULL;
            ret = 0;
        } else {
            fprintf(stderr, "\n %s %d mode get fail \n",__FUNCTION__,__LINE__);
        }
    } else {
        fprintf(stderr, "\n %s %d conn create fail \n",__FUNCTION__,__LINE__);
    }
    if (conn)
        mesonConnectorDestroy(drmFd,conn);
    return ret;
}
int meson_drm_changeMode(int drmFd, drmModeAtomicReq *req, DisplayMode* modeInfo, MESON_CONNECTOR_TYPE connType)
{
    int ret = -1;
    struct mesonConnector* conn = NULL;
    drmModeModeInfo drm_mode;
    int i;
    bool interlace = false;
    bool found = false;
    int rc = -1;
    int rc1 = -1;
    int rc2 = -1;
    int rc3 = -1;
    uint32_t connId;
    uint32_t crtcId;

    uint32_t blobId = 0;
    drmModeModeInfo* modes = NULL;
    int modesNumber = 0;

    if (modeInfo == NULL || drmFd < 0 || req == NULL) {
        fprintf(stderr, "\n %s %d invalid parameter return\n",__FUNCTION__,__LINE__);
        return ret;
    }
    conn = get_current_connector(drmFd, connType);
    connId = mesonConnectorGetId(conn);
    crtcId = mesonConnectorGetCRTCId(conn);
    if ( conn ) {
        mesonConnectorGetModes(conn, drmFd, &modes, &modesNumber);
        for ( i = 0; i < modesNumber; i++ ) {
            interlace = (modes[i].flags & DRM_MODE_FLAG_INTERLACE);
            if ( (modeInfo->w == modes[i].hdisplay)
                && (modeInfo->h == modes[i].vdisplay)
                && (modeInfo->vrefresh == modes[i].vrefresh)
                && (interlace == modeInfo->interlace)
            ) {
                drm_mode = modes[i];
                found = true;
                break;
            }
        }
    } else {
        fprintf(stderr, "\n %s %d conn create fail \n",__FUNCTION__,__LINE__);
    }

    if (found) {
        rc1 = meson_drm_set_property(drmFd, req, connId, DRM_MODE_OBJECT_CONNECTOR, "CRTC_ID", crtcId);
        rc = drmModeCreatePropertyBlob( drmFd, &drm_mode, sizeof(drm_mode), &blobId );
        if (rc == 0) {
            rc2 = meson_drm_set_property(drmFd, req, crtcId, DRM_MODE_OBJECT_CRTC, "MODE_ID", blobId);
            rc3 = meson_drm_set_property(drmFd, req, crtcId, DRM_MODE_OBJECT_CRTC, "ACTIVE", 1);
            fprintf(stderr, "\n %s %d  rc1:%d rc:%d rc2:%d, rc3:%d\n",__FUNCTION__,__LINE__, rc1, rc,rc2,rc3);
            if (rc1 >= 0 && rc2 >= 0 && rc3 >= 0)
                ret = 0;
        }
    }
    if (conn)
        mesonConnectorDestroy(drmFd,conn);
    return ret;
}

ENUM_MESON_CONN_CONNECTION meson_drm_getConnectionStatus(int drmFd, MESON_CONNECTOR_TYPE connType)
{
    ENUM_MESON_CONN_CONNECTION ret = MESON_UNKNOWNCONNECTION;
    struct mesonConnector* conn = NULL;
    if ( drmFd < 0) {
        fprintf(stderr, "\n%s %d drmFd < 0\n",__FUNCTION__,__LINE__);
        return ret;
    }
    conn = get_current_connector(drmFd, connType);
    if (conn) {
        int ConnectState = -1;
        ConnectState = mesonConnectorGetConnectState(conn);
        if (ConnectState == 1) {
            ret = MESON_CONNECTED;
        } else if (ConnectState == 2) {
            ret = MESON_DISCONNECTED;
        } else {
            ret = MESON_UNKNOWNCONNECTION;
        }
    } else {
        fprintf(stderr, "\n drm open fail\n");
    }
    if (conn)
        mesonConnectorDestroy(drmFd,conn);
    return ret;
}

ENUM_MESON_COLOR_SPACE meson_drm_getColorSpace(int drmFd, MESON_CONNECTOR_TYPE connType )
{
    char propName[PROP_NAME_MAX_LEN] = {'\0'};
    sprintf( propName, "%s", DRM_CONNECTOR_PROP_COLOR_SPACE);
    uint32_t value = 0;
    ENUM_MESON_COLOR_SPACE colorSpace = MESON_COLOR_SPACE_RESERVED;
    if ( drmFd < 0) {
        fprintf(stderr, "\n%s %d drmFd < 0\n",__FUNCTION__,__LINE__);
        return colorSpace;
    }
    if ( 0 == meson_drm_get_conn_prop_value(drmFd, connType, propName, &value )) {
        switch (value)
        {
            case 0:
                colorSpace = MESON_COLOR_SPACE_RGB;
                break;
            case 1:
                colorSpace = MESON_COLOR_SPACE_YCBCR422;
                break;
            case 2:
                colorSpace = MESON_COLOR_SPACE_YCBCR444;
                break;
            case 3:
                colorSpace = MESON_COLOR_SPACE_YCBCR420;
                break;
            default:
                colorSpace = MESON_COLOR_SPACE_RESERVED;
                break;
        }
    } else {
        fprintf(stderr, "\n%s %d fail\n",__FUNCTION__,__LINE__);
    }
    return colorSpace;
}
int meson_drm_setColorSpace(int drmFd, drmModeAtomicReq *req,
                       ENUM_MESON_COLOR_SPACE colorSpace, MESON_CONNECTOR_TYPE connType)
{
    int ret = -1;
    uint32_t connId = 0;
    int rc = -1;
    struct mesonConnector* conn = NULL;
    if ( drmFd < 0 || req == NULL) {
        fprintf(stderr, "\n %s %d invalid parameter return\n",__FUNCTION__,__LINE__);
        return ret;
    }
    conn = get_current_connector(drmFd, connType);
    if (conn) {
        connId = mesonConnectorGetId(conn);
        rc = meson_drm_set_property(drmFd, req, connId, DRM_MODE_OBJECT_CONNECTOR,
            DRM_CONNECTOR_PROP_COLOR_SPACE, (uint64_t)colorSpace);
        mesonConnectorDestroy(drmFd,conn);
    }
    if (rc >= 0)
        ret = 0;
    return ret;

}

uint32_t meson_drm_getColorDepth( int drmFd, MESON_CONNECTOR_TYPE connType )
{
    char propName[PROP_NAME_MAX_LEN] = {'\0'};
    sprintf( propName, "%s", DRM_CONNECTOR_PROP_COLOR_DEPTH);
    uint32_t value = 0;
    if ( drmFd < 0) {
        fprintf(stderr, "\n%s %d drmFd < 0\n",__FUNCTION__,__LINE__);
        return value;
    }
    if ( 0 != meson_drm_get_conn_prop_value( drmFd, connType, propName, &value )) {
         fprintf(stderr, "\n%s %d fail\n",__FUNCTION__,__LINE__);
    }
    return value;
}
int meson_drm_setColorDepth(int drmFd, drmModeAtomicReq *req,
                       uint32_t colorDepth, MESON_CONNECTOR_TYPE connType)
{
    int ret = -1;
    int rc = -1;
    struct mesonConnector* conn = NULL;
    uint32_t connId = 0;
    conn = get_current_connector(drmFd, connType);
    if ( drmFd < 0 || req == NULL) {
        fprintf(stderr, "\n %s %d invalid parameter return\n",__FUNCTION__,__LINE__);
        return ret;
    }
    if (conn) {
        connId = mesonConnectorGetId(conn);
        rc = meson_drm_set_property(drmFd, req, connId, DRM_MODE_OBJECT_CONNECTOR,
                       DRM_CONNECTOR_PROP_COLOR_DEPTH, (uint64_t)colorDepth);
        mesonConnectorDestroy(drmFd,conn);
    }
    if (rc >= 0)
        ret = 0;
    return ret;
}
ENUM_MESON_HDR_POLICY meson_drm_getHDRPolicy( int drmFd, MESON_CONNECTOR_TYPE connType )
{
    char propName[PROP_NAME_MAX_LEN] = {'\0'};
    sprintf( propName, "%s", DRM_CONNECTOR_PROP_TX_HDR_POLICY);
    uint32_t value = 0;
    ENUM_MESON_HDR_POLICY hdrPolicy = MESON_HDR_POLICY_FOLLOW_SINK;
    if ( drmFd < 0) {
        fprintf(stderr, "\n%s %d drmFd < 0\n",__FUNCTION__,__LINE__);
        return hdrPolicy;
    }
    if ( 0 == meson_drm_get_crtc_prop_value( drmFd, connType, propName, &value )) {
        if (value == 0)
            hdrPolicy = MESON_HDR_POLICY_FOLLOW_SINK;
        if (value == 1)
            hdrPolicy = MESON_HDR_POLICY_FOLLOW_SOURCE;
    }
    return hdrPolicy;
}

int meson_drm_setHDRPolicy(int drmFd, drmModeAtomicReq *req,
                       ENUM_MESON_HDR_POLICY hdrPolicy, MESON_CONNECTOR_TYPE connType)
{
    int ret = -1;
    int rc = -1;
    struct mesonConnector* conn = NULL;
    uint32_t crtcId = 0;
    conn = get_current_connector(drmFd, connType);
    if ( drmFd < 0 || req == NULL) {
        fprintf(stderr, "\n %s %d invalid parameter return\n",__FUNCTION__,__LINE__);
        return ret;
    }
    if (conn) {
        crtcId = mesonConnectorGetCRTCId(conn);
        rc = meson_drm_set_property(drmFd, req, crtcId, DRM_MODE_OBJECT_CRTC,
                       DRM_CONNECTOR_PROP_TX_HDR_POLICY, (uint64_t)hdrPolicy);
        mesonConnectorDestroy(drmFd,conn);
    }
    if (rc >= 0)
        ret = 0;
    return ret;
}

ENUM_MESON_HDCP_VERSION meson_drm_getHdcpVersion( int drmFd, MESON_CONNECTOR_TYPE connType )
{
    char propName[PROP_NAME_MAX_LEN] = {'\0'};
    sprintf( propName, "%s", DRM_CONNECTOR_PROP_TX_HDCP_AUTH_MODE);
    uint32_t value = 0;
    ENUM_MESON_HDCP_VERSION hdcpVersion = MESON_HDCP_RESERVED;
    if ( drmFd < 0) {
        fprintf(stderr, "\n%s %d drmFd < 0\n",__FUNCTION__,__LINE__);
        return hdcpVersion;
    }
    if ( 0 == meson_drm_get_conn_prop_value( drmFd, connType, propName, &value )) {
        if (value & 0x1)
            hdcpVersion = MESON_HDCP_14;
        if (value & 0x2)
            hdcpVersion = MESON_HDCP_22;
    }
    return hdcpVersion;
}

ENUM_MESON_HDR_MODE meson_drm_getHdrStatus(int drmFd, MESON_CONNECTOR_TYPE connType )
{
    char propName[PROP_NAME_MAX_LEN] = {'\0'};
    sprintf( propName, "%s", DRM_CONNECTOR_PROP_TX_HDR_MODE);
    uint32_t value = 0;
    ENUM_MESON_HDR_MODE hdrMode = MESON_SDR;
    if ( drmFd < 0) {
        fprintf(stderr, "\n%s %d drmFd < 0\n",__FUNCTION__,__LINE__);
        return hdrMode;
    }
    if ( 0 == meson_drm_get_conn_prop_value( drmFd, connType, propName, &value )) {
        switch (value)
        {
            case 0:
                hdrMode = MESON_HDR10PLUS;
                break;
            case 1:
                hdrMode = MESON_DOLBYVISION_STD;
                break;
            case 2:
                hdrMode = MESON_DOLBYVISION_LL;
                break;
            case 3:
                hdrMode = MESON_HDR10_ST2084;
                break;
            case 4:
                hdrMode = MESON_HDR10_TRADITIONAL;
                break;
            case 5:
                hdrMode = MESON_HDR_HLG;
                break;
            case 6:
                hdrMode = MESON_SDR;
                break;
            default:
                hdrMode = MESON_SDR;
                break;
        }
    } else {
        fprintf(stderr, "\n%s %d fail\n",__FUNCTION__,__LINE__);
    }
    return hdrMode;
}
void meson_drm_getEDIDData(int drmFd, MESON_CONNECTOR_TYPE connType, int * data_Len, char **data )
{
    int i = 0;
    int count = 0;
    char* edid_data = NULL;
    struct mesonConnector* conn = NULL;
    if (drmFd < 0 || data_Len == NULL || data == NULL) {
        fprintf(stderr, "\n %s %d invalid parameter return\n",__FUNCTION__,__LINE__);
        return;
    }
    conn = get_current_connector(drmFd, connType);
    if (conn == NULL) {
        fprintf(stderr, "\n%s %d connector create fail.return \n",__FUNCTION__,__LINE__);
        return;
    }
    if (0 != mesonConnectorGetEdidBlob(conn, &count, &edid_data))
        goto out;
    char* edid =  (char*)calloc(count, sizeof(char));
    if (edid == NULL) {
        fprintf(stderr, "\n%s %d edid alloc mem fail.return\n",__FUNCTION__,__LINE__);
        return;
    }
    for (i = 0; i < count; i++)
    {
        edid[i] = edid_data[i];
    }
    *data_Len = count;
    *data = edid;
out:
    if (conn)
        mesonConnectorDestroy(drmFd,conn);
}

int meson_drm_setAVMute(int drmFd, drmModeAtomicReq *req,
                       bool mute, MESON_CONNECTOR_TYPE connType)

{
    int ret = -1;
    int rc = -1;
    struct mesonConnector* conn = NULL;
    uint32_t connId = 0;
    if ( drmFd < 0 || req == NULL) {
        fprintf(stderr, "\n %s %d invalid parameter return\n",__FUNCTION__,__LINE__);
        return ret;
    }
    conn = get_current_connector(drmFd, connType);
    if (conn) {
        connId = mesonConnectorGetId(conn);
        rc = meson_drm_set_property(drmFd, req, connId, DRM_MODE_OBJECT_CONNECTOR,
                       MESON_DRM_HDMITX_PROP_AVMUTE, (uint64_t)mute);
        mesonConnectorDestroy(drmFd,conn);
    }
    if (rc >= 0)
        ret = 0;
    return ret;
}
ENUM_MESON_HDCPAUTH_STATUS meson_drm_getHdcpAuthStatus( int drmFd, MESON_CONNECTOR_TYPE connType )
{
    char propName[PROP_NAME_MAX_LEN] = {'\0'};
    sprintf( propName, "%s", DRM_CONNECTOR_PROP_CONTENT_PROTECTION);
    uint32_t value = 0;
    ENUM_MESON_HDCPAUTH_STATUS hdcpAuthStatus = MESON_AUTH_STATUS_FAIL;
    if ( drmFd < 0 ) {
        fprintf(stderr, "\n %s %d invalid parameter return\n",__FUNCTION__,__LINE__);
        return hdcpAuthStatus;
    }
    if ( 0 == meson_drm_get_conn_prop_value( drmFd, connType, propName, &value )) {
        if (value == 2)
            hdcpAuthStatus = MESON_AUTH_STATUS_SUCCESS;
    }
    return hdcpAuthStatus;
}

int meson_drm_setHDCPEnable(int drmFd, drmModeAtomicReq *req,
                       bool enable, MESON_CONNECTOR_TYPE connType)

{
    int ret = -1;
    int rc = -1;
    struct mesonConnector* conn = NULL;
    uint32_t connId = 0;
    if ( drmFd < 0 || req == NULL) {
        fprintf(stderr, "\n %s %d invalid parameter return\n",__FUNCTION__,__LINE__);
        return ret;
    }
    conn = get_current_connector(drmFd, connType);
    if (conn) {
        connId = mesonConnectorGetId(conn);
        rc = meson_drm_set_property(drmFd, req, connId, DRM_MODE_OBJECT_CONNECTOR,
                       DRM_CONNECTOR_PROP_CONTENT_PROTECTION, (uint64_t)enable);
        mesonConnectorDestroy(drmFd,conn);
    }
    if (rc >= 0)
        ret = 0;
    return ret;
}



