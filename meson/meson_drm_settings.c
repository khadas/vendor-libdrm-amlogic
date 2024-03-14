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
#include "meson_drm_log.h"
#include "meson_drm.h"

#define DEFAULT_CARD "/dev/dri/card0"
#define PROP_NAME_MAX_LEN 50
static int meson_drm_get_crtc_prop_value( int drmFd, MESON_CONNECTOR_TYPE connType,
        char* name, uint32_t* prop_value );
static int meson_drm_get_conn_prop_value( int drmFd, MESON_CONNECTOR_TYPE connType,
                                          char* name, uint32_t* propValue );
static int meson_drm_get_prop_value(int drmFd, MESON_CONNECTOR_TYPE connType,
                                     uint32_t objType, char* name, uint32_t* propValue );

static struct mesonConnector* get_current_connector(int drmFd, MESON_CONNECTOR_TYPE connType);
static struct mesonConnector* get_default_connector(int drmFd);

static int meson_drm_set_property(int drmFd, drmModeAtomicReq *req, uint32_t objId,
                                  uint32_t objType, char* name, uint64_t value);

static int meson_drm_get_prop_value(int drmFd, MESON_CONNECTOR_TYPE connType, uint32_t objType, char* name, uint32_t* propValue )
{
    int ret = -1;
    int objID = -1;
    struct mesonConnector* conn = NULL;
    if ( drmFd < 0 || name == NULL || propValue == NULL)
    {
        ERROR(" %s %d drmfd invalid, or property name invalid",__FUNCTION__,__LINE__);
        goto out;
    }
    conn = get_current_connector(drmFd, connType);
    if ( conn == NULL )
    {
        ERROR("%s %d get_current_connector fail",__FUNCTION__,__LINE__);
        goto out;
    }
    objID =  mesonConnectorGetId(conn);
    if (objType == DRM_MODE_OBJECT_CRTC)
        objID =  mesonConnectorGetCRTCId(conn);
    struct mesonProperty* meson_prop = NULL;
    meson_prop = mesonPropertyCreate(drmFd, objID, objType, name);
    if (!meson_prop) {
        ERROR("%s %d meson_prop create fail",__FUNCTION__,__LINE__);
        goto out;
    }
    uint64_t value = mesonPropertyGetValue(meson_prop);
    *propValue = (uint32_t)value;
    DEBUG("%s %d prop value:%llu objID:%d,name:%s",__FUNCTION__,__LINE__,value, objID,name);
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
        ERROR("%s %d drmFd < 0",__FUNCTION__,__LINE__);
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
            break;
        case MESON_CONNECTOR_DUMMY:
            drmConnType = DRM_MODE_CONNECTOR_VIRTUAL;
            break;
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
        ERROR("%s %d drm card:%s open fail",__FUNCTION__,__LINE__,card);
    else
        drmDropMaster(ret_fd);
    ret = drmSetClientCap(ret_fd, DRM_CLIENT_CAP_ATOMIC, 1);
    if (ret < 0)
        DEBUG("%s %d Unable to set DRM atomic capability",__FUNCTION__,__LINE__);

    ret = drmSetClientCap(ret_fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);
    if (ret < 0)
        DEBUG("%s %d Unable to set UNIVERSAL_PLANES",__FUNCTION__,__LINE__);
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
        ERROR(" %s %d invalid parameter return",__FUNCTION__,__LINE__);
        return rc;
    }
    struct mesonProperty *prop = NULL;
    prop = mesonPropertyCreate(drmFd, objId, objType, name);
    if (prop) {
        propId = mesonPropertyGetId(prop);
        mesonPropertyDestroy(prop);
        DEBUG("%s %d name:%s objId:%d propId:%d value:%llu", __FUNCTION__,__LINE__,name, objId, propId, value);
        rc = drmModeAtomicAddProperty( req, objId, propId, value );
        if (rc < 0)
            ERROR("%s %d drmModeAtomicAddProperty fail",__FUNCTION__,__LINE__);
        return rc;
    }
    return rc;
}

int meson_drm_getPreferredMode( DisplayMode* mode, MESON_CONNECTOR_TYPE connType) {
    int ret = -1;
    int i = 0;
    int count = 0;
    drmModeModeInfo* modes = NULL;
    int drmFd = -1;
    struct mesonConnector* conn = NULL;
    drmFd = meson_open_drm();
    conn = get_current_connector(drmFd, connType);
    if (conn == NULL || drmFd < 0)
    {
        ERROR("%s %d invalid parameter return",__FUNCTION__,__LINE__);
    }
    if (0 != mesonConnectorGetModes(conn, drmFd, &modes, &count))
        goto out;
    for (i = 0; i < count; i++)
    {
        if (modes[i].type & DRM_MODE_TYPE_PREFERRED)
        {
            mode->w = modes[i].hdisplay;
            mode->h = modes[i].vdisplay;
            mode->interlace = (modes[i].flags & DRM_MODE_FLAG_INTERLACE) ? true : false;
            strcpy(mode->name, modes[i].name );
            break;
        }
    }
    ret = 0;
out:
    if (conn)
        mesonConnectorDestroy(drmFd,conn);
    if (drmFd >= 0 )
        close(drmFd);
    DEBUG("%s %d get preferred mode%s %dx%d%s%dhz",__FUNCTION__,__LINE__, mode->name, mode->w, mode->h, (mode->interlace == 0? "p":"i"),mode->vrefresh);
    return ret;
}

int meson_drm_getsupportedModesList(int drmFd, DisplayMode** modeInfo, int* modeCount,MESON_CONNECTOR_TYPE connType )
{
    int ret = -1;
    struct mesonConnector* conn = NULL;
    if ( drmFd < 0) {
        ERROR("%s %d drmFd < 0",__FUNCTION__,__LINE__);
        return ret;
    }
    conn = get_current_connector(drmFd, connType);
    if (conn == NULL) {
        ERROR("%s %d connector create fail",__FUNCTION__,__LINE__);
        return ret;
    }
    drmModeModeInfo* modeall = NULL;
    int count = 0;
    int i = 0;
    if (0 != mesonConnectorGetModes(conn, drmFd, &modeall, &count))
        goto out;
    DisplayMode* modestemp =  (DisplayMode*)calloc(count, sizeof(DisplayMode));
    for (i = 0; i < count; i++)
    {
        modestemp[i].w = modeall[i].hdisplay;
        modestemp[i].h = modeall[i].vdisplay;
        modestemp[i].vrefresh = modeall[i].vrefresh;
        modestemp[i].interlace = (modeall[i].flags & DRM_MODE_FLAG_INTERLACE) ? true : false;
        strcpy(modestemp[i].name, modeall[i].name );
    }
    *modeCount = count;
    *modeInfo = modestemp;
    ret = 0;
out:
    if (conn)
        mesonConnectorDestroy(drmFd,conn);
     DEBUG("%s %d mode count: %d",__FUNCTION__,__LINE__,(*modeCount));
     for (int i=0; i < (*modeCount); i++) {
        DEBUG_EDID(" %s %dx%d%s%dhz\n", (*modeInfo)[i].name, (*modeInfo)[i].w, (*modeInfo)[i].h, ((*modeInfo)[i].interlace == 0? "p":"i"), (*modeInfo)[i].vrefresh);
    }
    return ret;
}

struct mesonConnector* get_default_connector(int drmFd)
{
    struct mesonConnector* connectorHDMI = NULL;
    struct mesonConnector* connectorLVDS = NULL;
    struct mesonConnector* connectorCVBS = NULL;
    int HDMIconnected = 0;
    int LVDSConnected = 0;
    int CVBSConnected = 0;

    connectorHDMI = mesonConnectorCreate(drmFd, DRM_MODE_CONNECTOR_HDMIA);
    if (connectorHDMI)
        HDMIconnected = mesonConnectorGetConnectState(connectorHDMI);
    if (HDMIconnected == 1) {
        return connectorHDMI;
    } else {
        mesonConnectorDestroy(drmFd,connectorHDMI);
        connectorLVDS = mesonConnectorCreate(drmFd, DRM_MODE_CONNECTOR_LVDS);
        if (connectorLVDS)
            LVDSConnected = mesonConnectorGetConnectState(connectorLVDS);
        if (LVDSConnected == 1) {
            return connectorLVDS;
        } else {
            mesonConnectorDestroy(drmFd,connectorLVDS);
            connectorCVBS = mesonConnectorCreate(drmFd, DRM_MODE_CONNECTOR_TV);
            if (connectorCVBS)
                CVBSConnected = mesonConnectorGetConnectState(connectorCVBS);
            if (CVBSConnected == 1) {
                return connectorCVBS;
            } else {
                mesonConnectorDestroy(drmFd, connectorCVBS);
                return NULL;
            }
        }
    }
}

int meson_drm_getModeInfo(int drmFd, MESON_CONNECTOR_TYPE connType, DisplayMode* modeInfo)
{
    int ret = -1;
    struct mesonConnector* conn = NULL;
    drmModeModeInfo* mode = NULL;
    if (modeInfo == NULL || drmFd < 0) {
        ERROR("%s %d modeInfo == NULL || drmFd < 0 return",__FUNCTION__,__LINE__);
        return ret;
    }
    if ( connType == MESON_CONNECTOR_RESERVED ) {
        conn = get_default_connector(drmFd);
        DEBUG("%s %d get default connector",__FUNCTION__,__LINE__);
    } else {
        conn = get_current_connector(drmFd, connType);
        DEBUG("%s %d get current connector",__FUNCTION__,__LINE__);
    }
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
            ERROR(" %s %d mode get fail ",__FUNCTION__,__LINE__);
        }
    } else {
        ERROR("%s %d conn create fail ",__FUNCTION__,__LINE__);
    }
    if (conn)
        mesonConnectorDestroy(drmFd,conn);
    DEBUG("%s %d modeInfo %dx%d%s%dhz",__FUNCTION__,__LINE__, modeInfo->w, modeInfo->h, (modeInfo->interlace == 0 ?"p":"i"), modeInfo->vrefresh);
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
    DEBUG("%s %d set modeInfo %dx%d%s%dhz connType %d",__FUNCTION__,__LINE__, modeInfo->w, modeInfo->h, (modeInfo->interlace == 0? "p":"i") , modeInfo->vrefresh,connType);
    if (modeInfo == NULL || drmFd < 0 || req == NULL) {
        ERROR(" %s %d invalid parameter return",__FUNCTION__,__LINE__);
        return ret;
    }
    if (connType == MESON_CONNECTOR_CVBS)
    {
        struct mesonConnector* connHDMI = NULL;
        uint32_t HDMIconnId;
        int rc4 = -1;
        connHDMI = get_current_connector(drmFd, MESON_CONNECTOR_HDMIA);
        HDMIconnId = mesonConnectorGetId(connHDMI);
        if (HDMIconnId) {
            rc4 = meson_drm_set_property(drmFd, req, HDMIconnId, DRM_MODE_OBJECT_CONNECTOR, "CRTC_ID", 0);
        }
        if (rc4 == -1) {
            ERROR("\n %s %d fail to disconnect HDMI \n", __FUNCTION__,__LINE__);
        } else {
            INFO(" %s %d change mode to cvbs, disconnect HDMI :%d ",__FUNCTION__,__LINE__,rc4);
        }
        mesonConnectorDestroy(drmFd,connHDMI);
    }
    conn = get_current_connector(drmFd, connType);
    connId = mesonConnectorGetId(conn);
    crtcId = mesonConnectorGetCRTCId(conn);
    if ( conn ) {
        DEBUG("%s %d conn create success",__FUNCTION__,__LINE__);
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
        ERROR(" %s %d conn create fail ",__FUNCTION__,__LINE__);
    }

    if (found) {
        DEBUG("%s %d find the corresponding modeinfo",__FUNCTION__,__LINE__);
        rc1 = meson_drm_set_property(drmFd, req, connId, DRM_MODE_OBJECT_CONNECTOR, "CRTC_ID", crtcId);
        rc = drmModeCreatePropertyBlob( drmFd, &drm_mode, sizeof(drm_mode), &blobId );
        if (rc == 0) {
            rc2 = meson_drm_set_property(drmFd, req, crtcId, DRM_MODE_OBJECT_CRTC, "MODE_ID", blobId);
            rc3 = meson_drm_set_property(drmFd, req, crtcId, DRM_MODE_OBJECT_CRTC, "ACTIVE", 1);
            DEBUG("%s %d  rc1:%d rc:%d rc2:%d, rc3:%d",__FUNCTION__,__LINE__, rc1, rc,rc2,rc3);
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
        ERROR( "%s %d drmFd < 0",__FUNCTION__,__LINE__);
        return ret;
    }
    conn = get_current_connector(drmFd, connType);
    if (conn) {
        DEBUG("%s %d conn create success",__FUNCTION__,__LINE__);
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
        ERROR(" %s %d conn create fail ",__FUNCTION__,__LINE__);
    }
    if (conn)
        mesonConnectorDestroy(drmFd,conn);
    DEBUG("%s %d get Connection Status %d",__FUNCTION__,__LINE__,ret);
    return ret;
}

ENUM_MESON_COLOR_SPACE meson_drm_getColorSpace(int drmFd, MESON_CONNECTOR_TYPE connType )
{
    char propName[PROP_NAME_MAX_LEN] = {'\0'};
    sprintf( propName, "%s", DRM_CONNECTOR_PROP_COLOR_SPACE);
    uint32_t value = 0;
    ENUM_MESON_COLOR_SPACE colorSpace = MESON_COLOR_SPACE_RESERVED;
    if ( drmFd < 0) {
        ERROR("%s %d drmFd < 0",__FUNCTION__,__LINE__);
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
        ERROR("%s %d get connector property value fail",__FUNCTION__,__LINE__);
    }
    DEBUG("%s %d get ColorSpace %d",__FUNCTION__,__LINE__,colorSpace);
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
        ERROR(" %s %d invalid parameter return",__FUNCTION__,__LINE__);
        return ret;
    }
    conn = get_current_connector(drmFd, connType);
    if (conn) {
        DEBUG("%s %d conn create success",__FUNCTION__,__LINE__);
        connId = mesonConnectorGetId(conn);
        rc = meson_drm_set_property(drmFd, req, connId, DRM_MODE_OBJECT_CONNECTOR,
            DRM_CONNECTOR_PROP_COLOR_SPACE, (uint64_t)colorSpace);
        mesonConnectorDestroy(drmFd,conn);
    }
    if (rc >= 0)
        ret = 0;
    DEBUG(" %s %d set colorSpace: %d",__FUNCTION__,__LINE__,colorSpace);
    return ret;

}

uint32_t meson_drm_getColorDepth( int drmFd, MESON_CONNECTOR_TYPE connType )
{
    char propName[PROP_NAME_MAX_LEN] = {'\0'};
    sprintf( propName, "%s", DRM_CONNECTOR_PROP_COLOR_DEPTH);
    uint32_t value = 0;
    if ( drmFd < 0) {
        ERROR("%s %d drmFd < 0",__FUNCTION__,__LINE__);
        return value;
    }
    if ( 0 != meson_drm_get_conn_prop_value( drmFd, connType, propName, &value )) {
         ERROR("%s %d get connector property value fail",__FUNCTION__,__LINE__);
    }
    DEBUG("%s %d get ColorDepth %d",__FUNCTION__,__LINE__,value);
    return value;
}
int meson_drm_setColorDepth(int drmFd, drmModeAtomicReq *req,
                       uint32_t colorDepth, MESON_CONNECTOR_TYPE connType)
{
    int ret = -1;
    int rc = -1;
    struct mesonConnector* conn = NULL;
    uint32_t connId = 0;
    if ( drmFd < 0 || req == NULL) {
        ERROR("%s %d invalid parameter return",__FUNCTION__,__LINE__);
        return ret;
    }
    conn = get_current_connector(drmFd, connType);
    if (conn) {
        DEBUG("%s %d get current connector success",__FUNCTION__,__LINE__);
        connId = mesonConnectorGetId(conn);
        rc = meson_drm_set_property(drmFd, req, connId, DRM_MODE_OBJECT_CONNECTOR,
                       DRM_CONNECTOR_PROP_COLOR_DEPTH, (uint64_t)colorDepth);
        mesonConnectorDestroy(drmFd,conn);
    }
    if (rc >= 0)
        ret = 0;
    DEBUG(" %s %d set colorDepth: %d",__FUNCTION__,__LINE__,colorDepth);
    return ret;
}
ENUM_MESON_HDR_POLICY meson_drm_getHDRPolicy( int drmFd, MESON_CONNECTOR_TYPE connType )
{
    char propName[PROP_NAME_MAX_LEN] = {'\0'};
    sprintf( propName, "%s", DRM_CONNECTOR_PROP_TX_HDR_POLICY);
    uint32_t value = 0;
    ENUM_MESON_HDR_POLICY hdrPolicy = MESON_HDR_POLICY_FOLLOW_SINK;
    if ( drmFd < 0) {
        ERROR( "%s %d drmFd < 0",__FUNCTION__,__LINE__);
        return hdrPolicy;
    }
    if ( 0 == meson_drm_get_crtc_prop_value( drmFd, connType, propName, &value )) {
        if (value == 0)
            hdrPolicy = MESON_HDR_POLICY_FOLLOW_SINK;
        if (value == 1)
            hdrPolicy = MESON_HDR_POLICY_FOLLOW_SOURCE;
        if (value == 2)
            hdrPolicy = MESON_HDR_POLICY_FOLLOW_FORCE_MODE;
    }
    DEBUG("%s %d get HDR Policy: %d",__FUNCTION__,__LINE__,hdrPolicy);
    return hdrPolicy;
}

int meson_drm_setHDRPolicy(int drmFd, drmModeAtomicReq *req,
                       ENUM_MESON_HDR_POLICY hdrPolicy, MESON_CONNECTOR_TYPE connType)
{
    int ret = -1;
    int rc = -1;
    struct mesonConnector* conn = NULL;
    uint32_t crtcId = 0;
    if ( drmFd < 0 || req == NULL) {
        ERROR("%s %d invalid parameter return",__FUNCTION__,__LINE__);
        return ret;
    }
    conn = get_current_connector(drmFd, connType);
    if (conn) {
        DEBUG("%s %d get current connector success",__FUNCTION__,__LINE__);
        crtcId = mesonConnectorGetCRTCId(conn);
        rc = meson_drm_set_property(drmFd, req, crtcId, DRM_MODE_OBJECT_CRTC,
                       DRM_CONNECTOR_PROP_TX_HDR_POLICY, (uint64_t)hdrPolicy);
        mesonConnectorDestroy(drmFd,conn);
    }
    if (rc >= 0)
        ret = 0;
    DEBUG("%s %d set hdrPolicy %d",__FUNCTION__,__LINE__,hdrPolicy);
    return ret;
}

ENUM_MESON_HDCP_VERSION meson_drm_getHdcpVersion( int drmFd, MESON_CONNECTOR_TYPE connType )
{
    char propName[PROP_NAME_MAX_LEN] = {'\0'};
    sprintf( propName, "%s", DRM_CONNECTOR_PROP_TX_HDCP_AUTH_MODE);
    uint32_t value = 0;
    ENUM_MESON_HDCP_VERSION hdcpVersion = MESON_HDCP_RESERVED;
    if ( drmFd < 0) {
        ERROR("%s %d drmFd < 0",__FUNCTION__,__LINE__);
        return hdcpVersion;
    }
    if ( 0 == meson_drm_get_conn_prop_value( drmFd, connType, propName, &value )) {
        if (value & 0x1)
            hdcpVersion = MESON_HDCP_14;
        if (value & 0x2)
            hdcpVersion = MESON_HDCP_22;
    }
    DEBUG("%s %d get Hdcp Version %d",__FUNCTION__,__LINE__,hdcpVersion);
    return hdcpVersion;
}

ENUM_MESON_HDR_MODE meson_drm_getHdrStatus(int drmFd, MESON_CONNECTOR_TYPE connType )
{
    char propName[PROP_NAME_MAX_LEN] = {'\0'};
    sprintf( propName, "%s", DRM_CONNECTOR_PROP_TX_HDR_MODE);
    uint32_t value = 0;
    ENUM_MESON_HDR_MODE hdrMode = MESON_SDR;
    if ( drmFd < 0) {
        ERROR("%s %d drmFd < 0",__FUNCTION__,__LINE__);
        return hdrMode;
    }
    if ( 0 == meson_drm_get_conn_prop_value( drmFd, connType, propName, &value )) {
        DEBUG("%s %d get conn prop value success",__FUNCTION__,__LINE__);
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
        ERROR("%s %d get connnetor property value fail",__FUNCTION__,__LINE__);
    }
    DEBUG("%s %d get HdrStatus %d",__FUNCTION__,__LINE__,hdrMode);
    return hdrMode;
}
void meson_drm_getEDIDData(int drmFd, MESON_CONNECTOR_TYPE connType, int * data_Len, char **data )
{
    int i = 0;
    int count = 0;
    char* edid_data = NULL;
    struct mesonConnector* conn = NULL;
    if (drmFd < 0 || data_Len == NULL || data == NULL) {
        ERROR("%s %d invalid parameter return",__FUNCTION__,__LINE__);
        return;
    }
    conn = get_current_connector(drmFd, connType);
    if (conn == NULL) {
        ERROR("%s %d connector create fail.return",__FUNCTION__,__LINE__);
        return;
    }
    if (0 != mesonConnectorGetEdidBlob(conn, &count, &edid_data))
        goto out;
    char* edid =  (char*)calloc(count, sizeof(char));
    if (edid == NULL) {
        ERROR("%s %d edid alloc mem fail.return",__FUNCTION__,__LINE__);
        return;
    }
    for (i = 0; i < count; i++)
    {
        edid[i] = edid_data[i];
    }
    *data_Len = count;
    *data = edid;
    DEBUG("%s %d data_Len: %d",__FUNCTION__,__LINE__, (*data_Len));
    for (int i = 0; i < (*data_Len); i++) {
        if (i % 16 == 0) {
            DEBUG_EDID("\n\t\t\t");
        }
        if (*data) {
            DEBUG_EDID("%.2hhx", (*data)[i]);
        }
    }
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
        ERROR(" %s %d invalid parameter return",__FUNCTION__,__LINE__);
        return ret;
    }
    conn = get_current_connector(drmFd, connType);
    if (conn) {
        DEBUG("%s %d get current connector success",__FUNCTION__,__LINE__);
        connId = mesonConnectorGetId(conn);
        rc = meson_drm_set_property(drmFd, req, connId, DRM_MODE_OBJECT_CONNECTOR,
                       MESON_DRM_HDMITX_PROP_AVMUTE, (uint64_t)mute);
        mesonConnectorDestroy(drmFd,conn);
    }
    if (rc >= 0)
        ret = 0;
    DEBUG(" %s %d set mute %d",__FUNCTION__,__LINE__,mute);
    return ret;
}

int meson_drm_getAVMute( int drmFd, MESON_CONNECTOR_TYPE connType )
{
    char propName[PROP_NAME_MAX_LEN] = {'\0'};
    sprintf( propName, "%s", MESON_DRM_HDMITX_PROP_AVMUTE);
    uint32_t value = 0;
    if ( drmFd < 0) {
        ERROR("%s %d drmFd < 0",__FUNCTION__,__LINE__);
        return value;
    }
    if ( 0 != meson_drm_get_conn_prop_value( drmFd, connType, propName, &value )) {
         ERROR("%s %d get connector property value fail",__FUNCTION__,__LINE__);
    }
    DEBUG("%s %d get avmute: %d (avmute control, 1 means set avmute, 0 means not set avmute)",__FUNCTION__,__LINE__,value);
    return value;
}

ENUM_MESON_HDCPAUTH_STATUS meson_drm_getHdcpAuthStatus( int drmFd, MESON_CONNECTOR_TYPE connType )
{
    char propName[PROP_NAME_MAX_LEN] = {'\0'};
    sprintf( propName, "%s", DRM_CONNECTOR_PROP_CONTENT_PROTECTION);
    uint32_t value = 0;
    ENUM_MESON_HDCPAUTH_STATUS hdcpAuthStatus = MESON_AUTH_STATUS_FAIL;
    if ( drmFd < 0 ) {
        ERROR("%s %d drmFd < 0",__FUNCTION__,__LINE__);
        return hdcpAuthStatus;
    }
    if ( 0 == meson_drm_get_conn_prop_value( drmFd, connType, propName, &value )) {
        if (value == 2)
            hdcpAuthStatus = MESON_AUTH_STATUS_SUCCESS;
    }
    DEBUG(" %s %d get hdcp auth status %d",__FUNCTION__,__LINE__,hdcpAuthStatus);
    return hdcpAuthStatus;
}

int meson_drm_setHDCPEnable(int drmFd, drmModeAtomicReq *req,
                       int enable, MESON_CONNECTOR_TYPE connType)
{
    int ret = -1;
    int rc = -1;
    struct mesonConnector* conn = NULL;
    uint32_t connId = 0;
    if ( drmFd < 0 || req == NULL) {
        ERROR("%s %d invalid parameter return",__FUNCTION__,__LINE__);
        return ret;
    }
    conn = get_current_connector(drmFd, connType);
    if (conn) {
        DEBUG("%s %d get current connector success",__FUNCTION__,__LINE__);
        connId = mesonConnectorGetId(conn);
        rc = meson_drm_set_property(drmFd, req, connId, DRM_MODE_OBJECT_CONNECTOR,
                       DRM_CONNECTOR_PROP_CONTENT_PROTECTION, (uint64_t)enable);
        mesonConnectorDestroy(drmFd,conn);
    }
    if (rc >= 0)
        ret = 0;
    DEBUG("%s %d set hdcp enable %d",__FUNCTION__,__LINE__,enable);
    return ret;
}

int meson_drm_setHDCPContentType(int drmFd, drmModeAtomicReq *req,
                       ENUM_MESON_HDCP_Content_Type HDCPType, MESON_CONNECTOR_TYPE connType)
{
    int ret = -1;
    int rc = -1;
    struct mesonConnector* conn = NULL;
    uint32_t connId = 0;
    if ( drmFd < 0 || req == NULL) {
        ERROR("%s %d invalid parameter return",__FUNCTION__,__LINE__);
        return ret;
    }
    conn = get_current_connector(drmFd, connType);
    if (conn) {
        DEBUG("%s %d get current connector success",__FUNCTION__,__LINE__);
        connId = mesonConnectorGetId(conn);
        rc = meson_drm_set_property(drmFd, req, connId, DRM_MODE_OBJECT_CONNECTOR,
                       DRM_CONNECTOR_PROP_CONTENT_TYPE, (uint64_t)HDCPType);
        mesonConnectorDestroy(drmFd,conn);
    }
    if (rc >= 0)
        ret = 0;
    DEBUG("%s %d set hdcp content %d",__FUNCTION__,__LINE__,HDCPType);
    return ret;
}

ENUM_MESON_HDCP_Content_Type meson_drm_getHDCPContentType( int drmFd, MESON_CONNECTOR_TYPE connType )
{
    char propName[PROP_NAME_MAX_LEN] = {'\0'};
    sprintf( propName, "%s", DRM_CONNECTOR_PROP_CONTENT_TYPE);
    uint32_t value = 0;
    ENUM_MESON_HDCP_Content_Type ContentType = MESON_HDCP_Type_RESERVED;
    if ( drmFd < 0) {
        ERROR("%s %d drmFd < 0",__FUNCTION__,__LINE__);
        return ContentType;
    }
    if ( 0 == meson_drm_get_conn_prop_value( drmFd, connType, propName, &value )) {
        switch (value)
        {
            case 0:
                ContentType = MESON_HDCP_Type0;
                break;
            case 1:
                ContentType = MESON_HDCP_Type1;
                break;
            default:
                ContentType = MESON_HDCP_Type_RESERVED;
                break;
        }
    } else {
        ERROR("%s %d get connector property value fail",__FUNCTION__,__LINE__);
    }
    DEBUG("%s %d get HDCP Content Type: %d",__FUNCTION__,__LINE__,ContentType);
    return ContentType;
}

MESON_CONTENT_TYPE meson_drm_getContentType(int drmFd, MESON_CONNECTOR_TYPE connType ) {

    char propName[PROP_NAME_MAX_LEN] = {'\0'};
    sprintf( propName, "%s", DRM_CONNECTOR_PROP_HDMI_CONTENT_TYPE);
    uint32_t value = 0;
    MESON_CONTENT_TYPE ContentType = MESON_CONTENT_TYPE_RESERVED;
    if ( drmFd < 0) {
        ERROR("%s %d drmFd < 0",__FUNCTION__,__LINE__);
        return ContentType;
    }
    if ( 0 == meson_drm_get_conn_prop_value(drmFd, connType, propName, &value )) {
        switch (value)
        {
            case 0:
                ContentType = MESON_CONTENT_TYPE_Data;
                break;
            case 1:
                ContentType = MESON_CONTENT_TYPE_Graphics;
                break;
            case 2:
                ContentType = MESON_CONTENT_TYPE_Photo;
                break;
            case 3:
                ContentType = MESON_CONTENT_TYPE_Cinema;
                break;
            case 4:
                ContentType = MESON_CONTENT_TYPE_Game;
                break;
            default:
                ContentType = MESON_CONTENT_TYPE_RESERVED;
                break;
        }
    } else {
        ERROR("%s %d get connector property value fail",__FUNCTION__,__LINE__);
    }
    DEBUG("%s %d get Content Type: %d",__FUNCTION__,__LINE__,ContentType);
    return ContentType;
}

int meson_drm_setDvEnable(int drmFd, drmModeAtomicReq *req,
                       uint32_t dvEnable, MESON_CONNECTOR_TYPE connType)
{
    int ret = -1;
    int rc = -1;
    struct mesonConnector* conn = NULL;
    uint32_t crtcId = 0;
    if ( drmFd < 0 || req == NULL) {
        ERROR(" %s %d invalid parameter return",__FUNCTION__,__LINE__);
        return ret;
    }
    conn = get_current_connector(drmFd, connType);
    if (conn) {
        DEBUG("%s %d get current connector success",__FUNCTION__,__LINE__);
        crtcId = mesonConnectorGetCRTCId(conn);
        rc = meson_drm_set_property(drmFd, req, crtcId, DRM_MODE_OBJECT_CRTC,
                       DRM_CONNECTOR_PROP_DV_ENABLE, (uint64_t)dvEnable);
        mesonConnectorDestroy(drmFd,conn);
    }
    if (rc >= 0)
        ret = 0;
    DEBUG("%s %d set dvEnable value %d",__FUNCTION__,__LINE__,dvEnable);
    return ret;
}

int meson_drm_getDvEnable( int drmFd, MESON_CONNECTOR_TYPE connType )
{
    char propName[PROP_NAME_MAX_LEN] = {'\0'};
    sprintf( propName, "%s", DRM_CONNECTOR_PROP_DV_ENABLE);
    uint32_t value = -1;
    if ( drmFd < 0) {
        ERROR("%s %d drmFd < 0",__FUNCTION__,__LINE__);
        return value;
    }
    if ( 0 != meson_drm_get_crtc_prop_value( drmFd, connType, propName, &value )) {
         ERROR("%s %d get crtc property value fail",__FUNCTION__,__LINE__);
    }
    DEBUG("%s %d get DvEnable %d",__FUNCTION__,__LINE__,value);
    return value;
}

int meson_drm_setActive(int drmFd, drmModeAtomicReq *req,
                       uint32_t active, MESON_CONNECTOR_TYPE connType)
{
    int ret = -1;
    int rc = -1;
    struct mesonConnector* conn = NULL;
    uint32_t crtcId = 0;
    if ( drmFd < 0 || req == NULL) {
        ERROR(" %s %d invalid parameter return",__FUNCTION__,__LINE__);
        return ret;
    }
    conn = get_current_connector(drmFd, connType);
    if (conn) {
        DEBUG("%s %d get current connector success",__FUNCTION__,__LINE__);
        crtcId = mesonConnectorGetCRTCId(conn);
        rc = meson_drm_set_property(drmFd, req, crtcId, DRM_MODE_OBJECT_CRTC,
                       DRM_CONNECTOR_PROP_ACTIVE, (uint64_t)active);
        mesonConnectorDestroy(drmFd,conn);
    }
    if (rc >= 0)
        ret = 0;
    DEBUG("%s %d set active %d",__FUNCTION__,__LINE__,active);
    return ret;
}

int meson_drm_getActive( int drmFd, MESON_CONNECTOR_TYPE connType )
{
    char propName[PROP_NAME_MAX_LEN] = {'\0'};
    sprintf( propName, "%s", DRM_CONNECTOR_PROP_ACTIVE);
    uint32_t value = 0;
    if ( drmFd < 0) {
        ERROR("%s %d drmFd < 0",__FUNCTION__,__LINE__);
        return value;
    }
    if ( 0 != meson_drm_get_crtc_prop_value( drmFd, connType, propName, &value )) {
         ERROR("%s %d get crtc property value fail",__FUNCTION__,__LINE__);
    }
    DEBUG("%s %d get Active: %d",__FUNCTION__,__LINE__,value);
    return value;
}

int meson_drm_setVrrEnabled(int drmFd, drmModeAtomicReq *req,
                       uint32_t VrrEnable, MESON_CONNECTOR_TYPE connType)
{
    int ret = -1;
    int rc = -1;
    struct mesonConnector* conn = NULL;
    uint32_t crtcId = 0;
    if ( drmFd < 0 || req == NULL) {
        ERROR("%s %d invalid parameter return",__FUNCTION__,__LINE__);
        return ret;
    }
    conn = get_current_connector(drmFd, connType);
    if (conn) {
        DEBUG("%s %d get current connector success",__FUNCTION__,__LINE__);
        crtcId = mesonConnectorGetCRTCId(conn);
        rc = meson_drm_set_property(drmFd, req, crtcId, DRM_MODE_OBJECT_CRTC,
                       DRM_CONNECTOR_VRR_ENABLED, (uint64_t)VrrEnable);
        mesonConnectorDestroy(drmFd,conn);
    }
    if (rc >= 0)
        ret = 0;
    DEBUG("%s %d set VrrEnable %d",__FUNCTION__,__LINE__,VrrEnable);
    return ret;
}

int meson_drm_getVrrEnabled( int drmFd, MESON_CONNECTOR_TYPE connType )
{
    char propName[PROP_NAME_MAX_LEN] = {'\0'};
    sprintf( propName, "%s", DRM_CONNECTOR_VRR_ENABLED);
    uint32_t value = 0;
    if ( drmFd < 0) {
        ERROR("%s %d drmFd < 0",__FUNCTION__,__LINE__);
        return value;
    }
    if ( 0 != meson_drm_get_crtc_prop_value( drmFd, connType, propName, &value )) {
         ERROR("%s %d get crtc property value fail",__FUNCTION__,__LINE__);
    }
    DEBUG("%s %d get VrrEnabled： %d",__FUNCTION__,__LINE__,value);
    return value;
}

uint32_t meson_drm_getHdrCap( int drmFd, MESON_CONNECTOR_TYPE connType )
{
    char propName[PROP_NAME_MAX_LEN] = {'\0'};
    sprintf( propName, "%s", DRM_CONNECTOR_PROP_RX_HDR_CAP);
    uint32_t value = 0;
    if ( drmFd < 0) {
        ERROR("%s %d drmFd < 0",__FUNCTION__,__LINE__);
        return value;
    }
    if ( 0 != meson_drm_get_conn_prop_value( drmFd, connType, propName, &value )) {
         ERROR("%s %d get connector property value fail",__FUNCTION__,__LINE__);
    }
    DEBUG("%s %d get hdrcap prop value %d",__FUNCTION__,__LINE__,value);
    return value;
}

uint32_t meson_drm_getDvCap( int drmFd, MESON_CONNECTOR_TYPE connType )
{
    char propName[PROP_NAME_MAX_LEN] = {'\0'};
    sprintf( propName, "%s", DRM_CONNECTOR_PROP_RX_DV_CAP);
    uint32_t value = 0;
    if ( drmFd < 0) {
        ERROR("%s %d drmFd < 0",__FUNCTION__,__LINE__);
        return value;
    }
    if ( 0 != meson_drm_get_conn_prop_value( drmFd, connType, propName, &value )) {
         ERROR("%s %d get connector property value fail",__FUNCTION__,__LINE__);
    }
    DEBUG("%s %d get dvcap：%d(presents the RX dolbyvision capability, [r] such as std or ll mode)",__FUNCTION__,__LINE__,value);
    return value;
}

int meson_drm_setVideoZorder(int drmFd, unsigned int index, unsigned int zorder, unsigned int flag) {
    int ret = -1;
    struct video_zpos zpos;
    if ( drmFd < 0) {
        ERROR("%s  %d drmFd < 0",__FUNCTION__,__LINE__);
        return ret;
    }
    zpos.flag = flag;
    zpos.index = index;
    zpos.zpos = zorder;
    ret = drmIoctl(drmFd, DRM_IOCTL_MESON_SET_VIDEO_ZPOS, &zpos);
    if (ret) {
        ERROR("\n failed to create object[%s].\n",strerror(errno));
    }
    return ret;
}

int meson_drm_setPlaneMute(int drmFd, unsigned int plane_type, unsigned int plane_mute)
{
    int ret = -1;
    struct drm_meson_plane_mute plane_info;
    if (drmFd < 0) {
        ERROR("%s  %d drmFd < 0",__FUNCTION__,__LINE__);
        return ret;
    }

    plane_info.plane_type = plane_type;
    plane_info.plane_mute = plane_mute;
    ret = drmIoctl(drmFd, DRM_IOCTL_MESON_MUTE_PLANE, &plane_info);
    if (ret)
        ERROR("\n failed to mute plane[%s].\n",strerror(errno));

    return ret;
}

ENUM_MESON_ASPECT_RATIO meson_drm_getAspectRatioValue( int drmFd, MESON_CONNECTOR_TYPE connType )
{
    char propName[PROP_NAME_MAX_LEN] = {'\0'};
    sprintf( propName, "%s", DRM_CONNECTOR_PROP_TX_ASPECT_RATIO);
    ENUM_MESON_ASPECT_RATIO ASPECTRATIO = MESON_ASPECT_RATIO_AUTOMATIC;
    uint32_t value = 0;
    if ( drmFd < 0) {
        ERROR("%s %d drmFd < 0",__FUNCTION__,__LINE__);
        return ASPECTRATIO;
    }
    if ( 0 == meson_drm_get_conn_prop_value( drmFd, connType, propName, &value )) {
        switch (value)
        {
            case 0:
                ASPECTRATIO = MESON_ASPECT_RATIO_AUTOMATIC;
                break;
            case 1:
                ASPECTRATIO = MESON_ASPECT_RATIO_4_3;
                break;
            case 2:
                ASPECTRATIO = MESON_ASPECT_RATIO_16_9;
                break;
            default:
                ASPECTRATIO = MESON_ASPECT_RATIO_RESERVED;
                break;
        }
    } else {
        ERROR(" %s %d get connector property value fail",__FUNCTION__,__LINE__);
    }
    DEBUG(" %s %d get aspect ratio value %d",__FUNCTION__,__LINE__, ASPECTRATIO);
    return ASPECTRATIO;
}

int meson_drm_setAspectRatioValue(int drmFd, drmModeAtomicReq *req,
                       ENUM_MESON_ASPECT_RATIO ASPECTRATIO, MESON_CONNECTOR_TYPE connType)
{
    int ret = -1;
    int rc = -1;
    struct mesonConnector* conn = NULL;
    uint32_t connId = 0;
    int value = meson_drm_getAspectRatioValue( drmFd, MESON_CONNECTOR_HDMIA );
    if (value == 0) {
        ERROR(" get current aspect ratio value：%d current mode do not support aspect ratio change", value);
    }
    if ( drmFd < 0 || req == NULL) {
        ERROR("%s %d invalid parameter return",__FUNCTION__,__LINE__);
        return ret;
    }
    conn = get_current_connector(drmFd, connType);
    if (conn) {
        DEBUG("%s %d get current connector success",__FUNCTION__,__LINE__);
        connId = mesonConnectorGetId(conn);
        rc = meson_drm_set_property(drmFd, req, connId, DRM_MODE_OBJECT_CONNECTOR,
                       DRM_CONNECTOR_PROP_TX_ASPECT_RATIO, (uint32_t)ASPECTRATIO);
        mesonConnectorDestroy(drmFd,conn);
    }
    if (rc >= 0)
        ret = 0;
    DEBUG("%s %d set aspect ratio value %d",__FUNCTION__,__LINE__,ASPECTRATIO);
    return ret;
}

int meson_drm_GetCrtcId(MESON_CONNECTOR_TYPE connType) {
    int drmFd = -1;
    int crtcId = -1;
    struct mesonConnector* conn = NULL;
    drmFd = meson_open_drm();
    conn = get_current_connector(drmFd, connType);
    if ( conn == NULL || drmFd < 0) {
        ERROR("%s %d invalid parameter return",__FUNCTION__,__LINE__);
        goto out;
    }
    crtcId = mesonConnectorGetCRTCId(conn);
out:
    if (conn) {
        mesonConnectorDestroy(drmFd,conn);
    }
    meson_close_drm(drmFd);
    return crtcId;
}

int meson_drm_GetConnectorId(MESON_CONNECTOR_TYPE connType) {
    int drmFd = -1;
    int connId = -1;
    struct mesonConnector* conn = NULL;
    drmFd = meson_open_drm();
    conn = get_current_connector(drmFd, connType);
    if ( conn == NULL || drmFd < 0) {
        ERROR("%s %d invalid parameter return",__FUNCTION__,__LINE__);
        goto out;
    }
    connId =  mesonConnectorGetId(conn);
out:
    if (conn) {
        mesonConnectorDestroy(drmFd,conn);
    }
    meson_close_drm(drmFd);
    return connId;
}

char* meson_drm_GetPropName( ENUM_MESON_DRM_PROP_NAME enProp) {
    char* propName = NULL;
    propName = (char*)malloc(sizeof(char)*50);
    if (propName == NULL) {
        ERROR("%s %d malloc fail",__FUNCTION__,__LINE__);
        return propName;
    } else {
        switch (enProp)
        {
            case ENUM_MESON_DRM_PROP_CONTENT_PROTECTION:
            {
                strcpy( propName, DRM_CONNECTOR_PROP_CONTENT_PROTECTION);
                break;
            }
            case ENUM_MESON_DRM_PROP_HDR_POLICY:
            {
                strcpy(propName,DRM_CONNECTOR_PROP_TX_HDR_POLICY);
                break;
            }
            case ENUM_MESON_DRM_PROP_HDMI_ENABLE:
            {
               strcpy(propName,MESON_DRM_HDMITX_PROP_AVMUTE);
               break;
            }
            case ENUM_MESON_DRM_PROP_COLOR_SPACE:
            {
               strcpy(propName,DRM_CONNECTOR_PROP_COLOR_SPACE);
               break;
            }
            case ENUM_MESON_DRM_PROP_COLOR_DEPTH:
            {
                strcpy(propName,DRM_CONNECTOR_PROP_COLOR_DEPTH);
                break;
            }
            case ENUM_MESON_DRM_PROP_HDCP_VERSION:
            {
                strcpy(propName,DRM_CONNECTOR_PROP_CONTENT_TYPE);
                break;
            }
            case ENUM_MESON_DRM_PROP_DOLBY_VISION_ENABLE:
            {
                strcpy(propName,DRM_CONNECTOR_PROP_DV_ENABLE);
                break;
            }
            case ENUM_MESON_DRM_PROP_ACTIVE:
            {
                strcpy(propName,DRM_CONNECTOR_PROP_ACTIVE);
                break;
            }
            case ENUM_MESON_DRM_PROP_VRR_ENABLED:
            {
                strcpy(propName,DRM_CONNECTOR_VRR_ENABLED);
                break;
            }
            case ENUM_MESON_DRM_PROP_ASPECT_RATIO:
            {
                strcpy(propName,DRM_CONNECTOR_PROP_TX_ASPECT_RATIO);
                break;
            }
            case ENUM_MESON_DRM_PROP_TX_HDR_OFF:
            {
                strcpy(propName, DRM_CONNECTOR_PROP_TX_HDR_OFF);
                break;
            }
            case ENUM_MESON_DRM_PROP_DV_MODE:
            {
                strcpy(propName,DRM_CONNECTOR_DV_MODE);
                break;
            }
            default:
                break;
        }
    }
    DEBUG("%s %d meson_drm_getprop_name：%s",__FUNCTION__,__LINE__, propName);
    return propName;
}

int meson_drm_setFracRatePolicy(int drmFd, drmModeAtomicReq *req,
                       uint32_t FracRate, MESON_CONNECTOR_TYPE connType)
{
    int ret = -1;
    int rc = -1;
    struct mesonConnector* conn = NULL;
    uint32_t connId = 0;
    if ( drmFd < 0 || req == NULL) {
        ERROR("%s %d invalid parameter return",__FUNCTION__,__LINE__);
        return ret;
    }
    conn = get_current_connector(drmFd, connType);
    if (conn) {
        DEBUG("%s %d get current connector success",__FUNCTION__,__LINE__);
        connId = mesonConnectorGetId(conn);
        rc = meson_drm_set_property(drmFd, req, connId, DRM_MODE_OBJECT_CONNECTOR,
                       DRM_CONNECTOR_FRAC_RATE_POLICY, (uint64_t)FracRate);
        mesonConnectorDestroy(drmFd,conn);
    }
    if (rc >= 0)
        ret = 0;
    DEBUG(" %s %d set Frac_Rate %d",__FUNCTION__,__LINE__,FracRate);
    return ret;
}

int meson_drm_getFracRatePolicy(int drmFd, MESON_CONNECTOR_TYPE connType)
{
    char propName[PROP_NAME_MAX_LEN] = {'\0'};
    sprintf( propName, "%s", DRM_CONNECTOR_FRAC_RATE_POLICY);
    uint32_t value = 0;
    if ( drmFd < 0) {
        ERROR("%s %d drmFd < 0",__FUNCTION__,__LINE__);
        return value;
    }
    if ( 0 != meson_drm_get_conn_prop_value( drmFd, connType, propName, &value )) {
         ERROR("%s %d get connector property value fail",__FUNCTION__,__LINE__);
    }
    DEBUG("%s %d get Frac_Rate %d",__FUNCTION__,__LINE__,value);
    return value;
}

int meson_drm_getHdrForceMode( int drmFd, MESON_CONNECTOR_TYPE connType )
{
    char propName[PROP_NAME_MAX_LEN] = {'\0'};
    sprintf( propName, "%s", DRM_CONNECTOR_PROP_TX_HDR_OFF);
    uint32_t value = 0;
    if ( drmFd < 0) {
        ERROR("%s %d drmFd < 0",__FUNCTION__,__LINE__);
        return value;
    }
    if ( 0 != meson_drm_get_crtc_prop_value( drmFd, connType, propName, &value )) {
         ERROR("%s %d get crtc property value fail",__FUNCTION__,__LINE__);
    }
    DEBUG("%s %d get force mode %d",__FUNCTION__,__LINE__,value);
    return value;
}

int meson_drm_setHdrForceMode(int drmFd, drmModeAtomicReq *req,ENUM_MESON_DRM_FORCE_MODE forcemode,
                                            MESON_CONNECTOR_TYPE connType)
{
    int ret = -1;
    int rc = -1;
    struct mesonConnector* conn = NULL;
    uint32_t crtcId = 0;
    if ( drmFd < 0 || req == NULL) {
        ERROR(" %s %d invalid parameter return",__FUNCTION__,__LINE__);
        return ret;
    }
    conn = get_current_connector(drmFd, connType);
    if (conn) {
        DEBUG("%s %d get current connector success",__FUNCTION__,__LINE__);
        crtcId = mesonConnectorGetCRTCId(conn);
        rc = meson_drm_set_property(drmFd, req, crtcId, DRM_MODE_OBJECT_CRTC,
                       DRM_CONNECTOR_PROP_TX_HDR_OFF, (uint64_t)forcemode);
        mesonConnectorDestroy(drmFd,conn);
    }
    if (rc >= 0)
        ret = 0;
    DEBUG("%s %d set force mode %d ",__FUNCTION__,__LINE__,forcemode);
    return ret;
}

int meson_drm_setDvMode(int drmFd, drmModeAtomicReq *req,
                       int dvMode, MESON_CONNECTOR_TYPE connType)
{
    int ret = -1;
    int rc = -1;
    struct mesonConnector* conn = NULL;
    int crtcId = 0;
    if ( drmFd < 0 || req == NULL) {
        ERROR(" %s %d invalid parameter return",__FUNCTION__,__LINE__);
        return ret;
    }
    conn = get_current_connector(drmFd, connType);
    if (conn) {
        DEBUG("%s %d get current connector success",__FUNCTION__,__LINE__);
        crtcId = mesonConnectorGetCRTCId(conn);
        rc = meson_drm_set_property(drmFd, req, crtcId, DRM_MODE_OBJECT_CRTC,
                       DRM_CONNECTOR_DV_MODE, (uint64_t)dvMode);
        mesonConnectorDestroy(drmFd,conn);
    }
    if (rc >= 0)
        ret = 0;
    DEBUG("%s %d set dv mode value %d",__FUNCTION__,__LINE__,dvMode);
    return ret;
}

int meson_drm_getDvMode( int drmFd, MESON_CONNECTOR_TYPE connType ) {
    char propName[PROP_NAME_MAX_LEN] = {'\0'};
    sprintf( propName, "%s", DRM_CONNECTOR_DV_MODE);
    uint32_t value = 0;
    if ( drmFd < 0) {
        ERROR("%s %d drmFd < 0",__FUNCTION__,__LINE__);
        return -1;
    }
    if ( 0 != meson_drm_get_crtc_prop_value( drmFd, connType, propName, &value )) {
         ERROR("%s %d get crtc property value fail",__FUNCTION__,__LINE__);
    }
    DEBUG("%s %d  get dv mode value %d",__FUNCTION__,__LINE__,value);
    return value;
}

int meson_drm_getDpmsStatus( int drmFd, MESON_CONNECTOR_TYPE connType )
{
    char propName[PROP_NAME_MAX_LEN] = {'\0'};
    sprintf( propName, "%s", DRM_CONNECTOR_PROP_DPMS);
    uint32_t value = 0;
    if ( drmFd < 0) {
        ERROR("%s %d drmFd < 0",__FUNCTION__,__LINE__);
        return value;
    }
    if ( 0 != meson_drm_get_conn_prop_value( drmFd, connType, propName, &value )) {
         ERROR("%s %d get connector property value fail",__FUNCTION__,__LINE__);
    }
    DEBUG("%s %d get dpms status %d",__FUNCTION__,__LINE__,value);
    return value;
}

int meson_drm_getGraphicPlaneSize(int drmFd, uint32_t* width, uint32_t* height) {
    int ret = -1;
    struct mesonPrimaryPlane* planesize = NULL;
    if (width == NULL || height == NULL) {
        ERROR("%s %d Error: One or both pointers are NULL.\n",__FUNCTION__,__LINE__);
        return ret;
    }
    if ( drmFd < 0) {
        ERROR("%s %d drmFd < 0",__FUNCTION__,__LINE__);
        return ret;
    }
    planesize = mesonPrimaryPlaneCreate(drmFd);
    if (planesize == NULL) {
        ERROR("%s %d connector create fail.return",__FUNCTION__,__LINE__);
        return ret;
    }
    if (mesonPrimaryPlaneGetFbSize(planesize, width, height) != 0) {
        goto out;
    }
    DEBUG("%s %d GetGraphicPlaneSize %d x %d",__FUNCTION__,__LINE__,*width, *height);
    ret = 0;
out:
    if (planesize)
        mesonPrimaryPlaneDestroy(drmFd,planesize);
    return ret;
}

int meson_drm_getPhysicalSize(int drmFd, uint32_t* width, uint32_t* height, MESON_CONNECTOR_TYPE connType) {
    int ret = -1;
    struct mesonConnector* conn = NULL;
    if (width == NULL || height == NULL) {
        ERROR("%s %d Error: One or both pointers are NULL.\n",__FUNCTION__,__LINE__);
        return ret;
    }
    if ( drmFd < 0) {
        ERROR("%s %d drmFd < 0",__FUNCTION__,__LINE__);
        return ret;
    }
    conn = get_current_connector(drmFd, connType);
    if ( conn == NULL) {
        ERROR("%s %d  connector create fail.return",__FUNCTION__,__LINE__);
        return ret;
    }
    if (mesonConnectorGetPhysicalSize(conn, width, height) != 0) {
        goto out;
    }
    DEBUG("%s %d GetPhysicalSize %d x %d",__FUNCTION__,__LINE__,*width, *height);
    ret = 0;
out:
    if (conn)
        mesonConnectorDestroy(drmFd,conn);
    return ret;
}

int meson_drm_getSignalTimingInfo(int drmFd, uint16_t* htotal, uint16_t* vtotal, uint16_t* hstart,
                                      uint16_t* vstart, MESON_CONNECTOR_TYPE connType) {
    int ret = -1;
    struct mesonConnector* conn = NULL;
    drmModeModeInfo* mode = NULL;
    if (htotal == NULL || vtotal == NULL || hstart == NULL || vstart == NULL || drmFd < 0) {
        ERROR(" %s %d invalid parameter return",__FUNCTION__,__LINE__);
        return ret;
    }
    conn = get_current_connector(drmFd, connType);
    if ( conn ) {
        mode = mesonConnectorGetCurMode(drmFd, conn);
        if (mode) {
            *htotal = mode->htotal;
            *vtotal = mode->vtotal;
            *hstart = mode->htotal - mode->hsync_end;
            *vstart = mode->vtotal - mode->vsync_end;
            free(mode);
            mode = NULL;
            ret = 0;
        } else {
            ERROR(" %s %d mode get fail ",__FUNCTION__,__LINE__);
        }
    } else {
        ERROR("%s %d conn create fail ",__FUNCTION__,__LINE__);
    }
    if (conn)
        mesonConnectorDestroy(drmFd,conn);
    DEBUG("%s %d htotal: %d vtotal: %d hstart: %d vstart: %d",__FUNCTION__,__LINE__,
                              *htotal,*vtotal,*hstart,*vstart);
    return ret;
}

