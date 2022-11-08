/*
 * Copyright (c) 2021 Amlogic, Inc. All rights reserved.
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
#include "meson_drm_display.h"


#define DEFAULT_CARD "/dev/dri/card0"
#ifndef XDG_RUNTIME_DIR
#define XDG_RUNTIME_DIR     "/run"
#endif

static int meson_drm_setprop(int obj_id, char* prop_name, int prop_value );
static uint32_t _getHDRSupportedList(uint64_t hdrlist, uint64_t dvlist);
struct mesonConnector* get_current_connector(int drmFd);
#define  FRAC_RATE_POLICY     "/sys/class/amhdmitx/amhdmitx0/frac_rate_policy"
static int  _amsysfs_get_sysfs_str(const char *path, char *valstr, int size);
static int  _get_frac_rate_policy();


static int meson_drm_setprop(int obj_id, char* prop_name, int prop_value )
{
    int ret = -1;
    printf(" meson_drm_setprop: obj_id %d, prop_name: %s, prop_value:%d\n",obj_id, prop_name,prop_value);
    char* xdgRunDir = getenv("XDG_RUNTIME_DIR");
    if (!xdgRunDir)
        xdgRunDir = XDG_RUNTIME_DIR;
    if (prop_name) {
        do {
            char cmdBuf[512] = {'\0'};
            snprintf(cmdBuf, sizeof(cmdBuf)-1, "export XDG_RUNTIME_DIR=%s;westeros-gl-console set property -s %d:%s:%d | grep \"Response\"",
                    xdgRunDir, obj_id, prop_name, prop_value);
            printf("Executing '%s'\n", cmdBuf);
            FILE* fp = popen(cmdBuf, "r");
            if (NULL != fp) {
                char output[64] = {'\0'};
                while (fgets(output, sizeof(output)-1, fp)) {
                    if (strlen(output) && strstr(output, "[0:")) {
                        ret = 0;
                        printf("\n meson_drm_setprop:%s\n",output);
                    }
                }
                pclose(fp);
            } else {
                printf("meson_drm_setprop: popen failed\n");
            }
            if (ret != 0 ) {
                if (strcmp(xdgRunDir, XDG_RUNTIME_DIR) == 0) {
                    break;
                }
                xdgRunDir = XDG_RUNTIME_DIR;
            }
        } while (ret != 0);
    }
    return ret;
}

static uint32_t _getHDRSupportedList(uint64_t hdrlist, uint64_t dvlist)
{
    uint32_t ret = 0;
    printf("\n _getHDRSupportedList hdrlist:%llu, dvlist:%llu\n", hdrlist, dvlist);
    if (!!(hdrlist & 0x1))
        ret = ret | (0x1 << (int)MESON_DRM_HDR10PLUS);

    if (!!(dvlist & 0x1A))
        ret = ret | (0x1 << (int)MESON_DRM_DOLBYVISION_STD);

    if (!!(dvlist & 0xE0))
        ret = ret | (0x1 << (int)MESON_DRM_DOLBYVISION_LL);

    if (!!(hdrlist & 0x8))
        ret = ret | (0x1 << (int)MESON_DRM_HDR10_ST2084);

    if (!!(hdrlist & 0x4))
        ret = ret | (0x1 << (int)MESON_DRM_HDR10_TRADITIONAL);

    if (!!(hdrlist & 0x10))
        ret = ret | (0x1 << (int)MESON_DRM_HDR_HLG);

    if (!!(hdrlist & 0x2))
        ret = ret | (0x1 << (int)MESON_DRM_SDR);

    return ret;
}

int meson_drm_setMode(DisplayMode* mode)
{
    int ret = -1;
    char cmdBuf[512] = {'\0'};
    char output[64] = {'\0'};
    char modeSet[20] = {'\0'};
    char* xdgRunDir = getenv("XDG_RUNTIME_DIR");
    if (!xdgRunDir)
        xdgRunDir = XDG_RUNTIME_DIR;
    if (!mode) {
        ret = -1;
    } else {
        sprintf(modeSet, "%dx%d%c%d", mode->w, mode->h, mode->interlace ? 'i':'p',mode->vrefresh);
        do {
            snprintf(cmdBuf, sizeof(cmdBuf)-1, "export XDG_RUNTIME_DIR=%s;westeros-gl-console set mode %s | grep \"Response\"",
                    xdgRunDir, modeSet);
            printf("Executing '%s'\n", cmdBuf);
            /* FIXME: popen in use */
            FILE* fp = popen(cmdBuf, "r");
            if (NULL != fp) {
                while (fgets(output, sizeof(output)-1, fp)) {
                    if (strlen(output) && strstr(output, "[0:")) {
                        ret = 0;
                    } else {
                        ret = -1;
                    }
                }
                pclose(fp);
            } else {
                printf(" popen failed\n");
                ret = -1;
            }
            if (ret != 0 ) {
                if (strcmp(xdgRunDir, XDG_RUNTIME_DIR) == 0) {
                    printf("meson_drm_setMode: failed !!\n");
                    break;
                }
                xdgRunDir = XDG_RUNTIME_DIR;
            }
        } while (ret != 0);
    }
    return ret;
}
int meson_drm_getMode(DisplayMode* modeInfo)
{
    int ret = -1;
    struct mesonConnector* conn = NULL;
    drmModeModeInfo* mode = NULL;
    int drmFd = -1;
    if (modeInfo == NULL) {
        printf("\n %s %d modeInfo == NULL return\n",__FUNCTION__,__LINE__);
        return ret;
    }
    drmFd = meson_drm_open();
    conn = mesonConnectorCreate(drmFd, DRM_MODE_CONNECTOR_HDMIA);
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
            printf("\n %s %d mode get fail \n",__FUNCTION__,__LINE__);
        }
    } else {
        printf("\n %s %d conn create fail \n",__FUNCTION__,__LINE__);
    }
    if (conn)
        mesonConnectorDestroy(drmFd,conn);
    if (drmFd >= 0 )
        close(drmFd);
    return ret;
}

int meson_drm_getRxSurportedModes( DisplayMode** modes, int* modeCount )
{
    int ret = -1;
    int drmFd = -1;
    struct mesonConnector* conn = NULL;
    drmFd = meson_drm_open();
    conn = mesonConnectorCreate(drmFd, DRM_MODE_CONNECTOR_HDMIA);
    if (conn == NULL || drmFd < 0)
    {
        printf("\n%s %d connector create fail\n",__FUNCTION__,__LINE__);
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
    *modes = modestemp;
    ret = 0;
out:
    if (conn)
        mesonConnectorDestroy(drmFd,conn);
    if (drmFd >= 0 )
        close(drmFd);
    return ret;
}
int meson_drm_getRxPreferredMode( DisplayMode* mode)
{
    int ret = -1;
    int i = 0;
    int count = 0;
    drmModeModeInfo* modes = NULL;
    int drmFd = -1;
    struct mesonConnector* conn = NULL;
    drmFd = meson_drm_open();
    conn = mesonConnectorCreate(drmFd, DRM_MODE_CONNECTOR_HDMIA);
    if (conn == NULL || drmFd < 0)
    {
        printf("\n%s %d connector create fail\n",__FUNCTION__,__LINE__);
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
    return ret;
}

int meson_drm_getEDID( int * data_Len, char **data)
{
    int ret = -1;
    int i = 0;
    int count = 0;
    char* edid_data = NULL;
    int drmFd = -1;
    struct mesonConnector* conn = NULL;
    drmFd = meson_drm_open();
    conn = mesonConnectorCreate(drmFd, DRM_MODE_CONNECTOR_HDMIA);
    if (conn == NULL || drmFd < 0)
    {
        printf("\n%s %d connector create fail\n",__FUNCTION__,__LINE__);
    }
    if (0 != mesonConnectorGetEdidBlob(conn, &count, &edid_data))
        goto out;
    char* edid =  (char*)calloc(count, sizeof(char));
    for (i = 0; i < count; i++)
    {
        edid[i] = edid_data[i];
    }
    *data_Len = count;
    *data = edid;
    ret = 0;
out:
    if (conn)
        mesonConnectorDestroy(drmFd,conn);
    if (drmFd >= 0 )
        close(drmFd);
    return ret;
}

int meson_drm_getRxSurportedEOTF(ENUM_DRM_HDMITX_PROP_EOTF* EOTFs)
{
    //get from EDID
    return 0;
}

ENUM_MESON_DRM_CONNECTION meson_drm_getConnection()
{
    ENUM_MESON_DRM_CONNECTION ret = MESON_DRM_UNKNOWNCONNECTION;
    int drmFd = -1;
    struct mesonConnector* conn = NULL;
    drmFd = meson_drm_open();
    conn = mesonConnectorCreate(drmFd, DRM_MODE_CONNECTOR_HDMIA);
    if (conn == NULL || drmFd < 0)
    {
        printf("\n%s %d connector create fail\n",__FUNCTION__,__LINE__);
    }
    if (conn) {
        int ConnectState = -1;
        ConnectState = mesonConnectorGetConnectState(conn);
        if (ConnectState == 1) {
            ret = MESON_DRM_CONNECTED;
        } else if (ConnectState == 2) {
            ret = MESON_DRM_DISCONNECTED;
        } else {
            ret = MESON_DRM_UNKNOWNCONNECTION;
        }
    } else {
        printf("\n drm open fail\n");
    }
    if (conn)
        mesonConnectorDestroy(drmFd,conn);
    if (drmFd >= 0 )
        close(drmFd);
    return ret;
}

int meson_drm_set_prop( ENUM_MESON_DRM_PROP enProp, int prop_value )
{
    int ret = -1;
    int objID = -1;
    char propName[50] = {'\0'};
    bool force1_4 = false;
    if (enProp >= ENUM_DRM_PROP_MAX) {
        printf("\n%s %d invalid para\n",__FUNCTION__,__LINE__);
        goto out;
    }
    int drmFd = -1;
    struct mesonConnector* conn = NULL;
    drmFd = meson_drm_open();
    conn = mesonConnectorCreate(drmFd, DRM_MODE_CONNECTOR_HDMIA);
    if (conn == NULL || drmFd < 0)
    {
        printf("\n%s %d connector create fail\n",__FUNCTION__,__LINE__);
        goto out;
    }
    switch (enProp)
    {
        case ENUM_DRM_PROP_HDMI_ENABLE:
        {
            objID =  mesonConnectorGetId(conn);
            sprintf( propName, "%s", MESON_DRM_HDMITX_PROP_AVMUTE);
            prop_value = prop_value ? 0:1;
            break;
        }
        case ENUM_DRM_PROP_HDMITX_EOTF:
        {
            objID =  mesonConnectorGetCRTCId(conn);
            sprintf( propName, "%s", MESON_DRM_HDMITX_PROP_EOTF);
            break;
        }
        case ENUM_DRM_PROP_CONTENT_PROTECTION:
        {
            objID =  mesonConnectorGetId(conn);
            sprintf( propName, "%s", DRM_CONNECTOR_PROP_CONTENT_PROTECTION);
            break;
        }
        case ENUM_DRM_PROP_HDCP_VERSION:
        {
            objID =  mesonConnectorGetId(conn);
            sprintf( propName, "%s", DRM_CONNECTOR_PROP_CONTENT_TYPE);
            if (ENUM_HDCP_VERSION_FORCE_1_4 == prop_value)
            {
                prop_value = 0;
                force1_4 = true;
            }
            break;
        }
        case ENUM_DRM_PROP_HDR_POLICY:
        {
            objID =  mesonConnectorGetCRTCId(conn);
            sprintf( propName, "%s", DRM_CONNECTOR_PROP_TX_HDR_POLICY);
            break;
        }
        case ENUM_DRM_PROP_HDMI_ASPECT_RATIO:
        {
            objID =  mesonConnectorGetId(conn);
            sprintf( propName, "%s", DRM_CONNECTOR_PROP_TX_ASPECT_RATIO);
            break;
        }
        case ENUM_DRM_PROP_HDMI_DV_ENABLE:
        {
            objID =  mesonConnectorGetCRTCId(conn);
            sprintf( propName, "%s", DRM_CONNECTOR_PROP_DV_ENABLE);
        }
        default:
            break;
    }
    meson_drm_setprop(objID, propName, prop_value);
    if (enProp == ENUM_DRM_PROP_HDCP_VERSION)
    {
        objID =  mesonConnectorGetId(conn);
        sprintf( propName, "%s", DRM_CONNECTOR_PROP_HDCP_PRIORITY);
        int priority = 0;
        if (force1_4)
        {
            priority = 1;
        }
        meson_drm_setprop(objID, propName, priority);
    }
    ret = 0;
out:
    if (conn)
        mesonConnectorDestroy(drmFd,conn);
    if (drmFd >= 0 )
        close(drmFd);
    return ret;
}

int meson_drm_get_prop( ENUM_MESON_DRM_PROP enProp, uint32_t* prop_value )
{
    int ret = -1;
    int objID = -1;
    int objtype = -1;
    char propName[50] = {'\0'};
    if (!prop_value || enProp >= ENUM_DRM_PROP_MAX) {
        printf("\n%s %d invalid para\n",__FUNCTION__,__LINE__);
        goto out;
    }
    int drmFd = -1;
    struct mesonConnector* conn = NULL;
    drmFd = meson_drm_open();
    conn = mesonConnectorCreate(drmFd, DRM_MODE_CONNECTOR_HDMIA);
    if (conn == NULL || drmFd < 0)
    {
        printf("\n%s %d connector create fail\n",__FUNCTION__,__LINE__);
        goto out;
    }
    switch (enProp)
    {
        case ENUM_DRM_PROP_HDMI_ENABLE:
        {
            objID =  mesonConnectorGetId(conn);
            objtype = DRM_MODE_OBJECT_CONNECTOR;
            sprintf( propName, "%s", MESON_DRM_HDMITX_PROP_AVMUTE);
            break;
        }
        case ENUM_DRM_PROP_HDMITX_EOTF:
        {
            objID =  mesonConnectorGetCRTCId(conn);
            objtype = DRM_MODE_OBJECT_CRTC;
            sprintf( propName, "%s", MESON_DRM_HDMITX_PROP_EOTF);
            break;
        }
        case ENUM_DRM_PROP_CONTENT_PROTECTION:
        {
            objID =  mesonConnectorGetId(conn);
            objtype = DRM_MODE_OBJECT_CONNECTOR;
            sprintf( propName, "%s", DRM_CONNECTOR_PROP_CONTENT_PROTECTION);
            break;
        }
        case ENUM_DRM_PROP_HDCP_VERSION:
        {
            objID =  mesonConnectorGetId(conn);
            objtype = DRM_MODE_OBJECT_CONNECTOR;
            sprintf( propName, "%s", DRM_CONNECTOR_PROP_CONTENT_TYPE);
            break;
        }
        case ENUM_DRM_PROP_HDR_POLICY:
        {
            objID =  mesonConnectorGetCRTCId(conn);
            sprintf( propName, "%s", DRM_CONNECTOR_PROP_TX_HDR_POLICY);
            objtype = DRM_MODE_OBJECT_CRTC;
            break;
        }
        case ENUM_DRM_PROP_GETRX_HDCP_SUPPORTED_VERS:
        {
            objID =  mesonConnectorGetId(conn);
            objtype = DRM_MODE_OBJECT_CONNECTOR;
            sprintf( propName, "%s", DRM_CONNECTOR_PROP_RX_HDCP_SUPPORTED_VER);
            break;
        }
        case ENUM_DRM_PROP_GETRX_HDR_CAP:
        {
            objID =  mesonConnectorGetId(conn);
            objtype = DRM_MODE_OBJECT_CONNECTOR;
            sprintf( propName, "%s", DRM_CONNECTOR_PROP_RX_HDR_CAP);
            break;
        }
        case ENUM_DRM_PROP_GETTX_HDR_MODE:
        {
            objID =  mesonConnectorGetId(conn);
            objtype = DRM_MODE_OBJECT_CONNECTOR;
            sprintf( propName, "%s", DRM_CONNECTOR_PROP_TX_HDR_MODE);
            break;
        }
        case ENUM_DRM_PROP_HDMI_ASPECT_RATIO:
        {
            objID =  mesonConnectorGetId(conn);
            objtype = DRM_MODE_OBJECT_CONNECTOR;
            sprintf( propName, "%s", DRM_CONNECTOR_PROP_TX_ASPECT_RATIO);
            break;
        }
        case ENUM_DRM_PROP_HDMI_DV_ENABLE:
        {
            objID =  mesonConnectorGetCRTCId(conn);
            objtype = DRM_MODE_OBJECT_CRTC;
            sprintf( propName, "%s", DRM_CONNECTOR_PROP_DV_ENABLE);
            break;
        }
        case ENUM_DRM_PROP_GETRX_HDCP_AUTHMODE:
        {
             objID =  mesonConnectorGetId(conn);
             objtype = DRM_MODE_OBJECT_CONNECTOR;
             sprintf( propName, "%s", DRM_CONNECTOR_PROP_TX_HDCP_AUTH_MODE);
             break;
        }
        default:
            break;
    }
    struct mesonProperty* meson_prop = NULL;
    meson_prop = mesonPropertyCreate(drmFd, objID, objtype, propName);
    if (!meson_prop) {
        printf("\n meson_prop create fail\n");
        goto out;
    }
    uint64_t value = mesonPropertyGetValue(meson_prop);
    printf("\n prop value:%llu objID:%d,name:%s\n",value, objID,propName);
    if (enProp == ENUM_DRM_PROP_HDMI_ENABLE)
        value = value ? 0:1;
    *prop_value = (uint32_t)value;

    if (enProp == ENUM_DRM_PROP_GETRX_HDR_CAP)
    {
        objID =  mesonConnectorGetId(conn);
        objtype = DRM_MODE_OBJECT_CONNECTOR;
        sprintf( propName, "%s", DRM_CONNECTOR_PROP_RX_DV_CAP);
        struct mesonProperty* meson_prop_dv = NULL;
        meson_prop_dv = mesonPropertyCreate(drmFd, objID, objtype, propName);
        uint64_t value_2 = mesonPropertyGetValue(meson_prop_dv);
        mesonPropertyDestroy(meson_prop_dv);
        *prop_value = _getHDRSupportedList(value, value_2);
    }
    if (enProp == ENUM_DRM_PROP_GETRX_HDCP_SUPPORTED_VERS)
    {
        *prop_value = 0;
        if (value == 14)
            *prop_value = 0x1 << 0;
        if (value == 22)
            *prop_value = 0x1 << 1;
        if (value == 36)
            *prop_value = 0x1 | (0x1 << 1);
    }
    if (enProp == ENUM_DRM_PROP_HDCP_VERSION)
    {
        if (*prop_value == 0)
        {
            objID =  mesonConnectorGetId(conn);
            objtype = DRM_MODE_OBJECT_CONNECTOR;
            sprintf( propName, "%s", DRM_CONNECTOR_PROP_HDCP_PRIORITY);
            struct mesonProperty* meson_prop_HDCP = NULL;
            meson_prop_HDCP = mesonPropertyCreate(drmFd, objID, objtype, propName);
            uint64_t value_3 = mesonPropertyGetValue(meson_prop_HDCP);
            mesonPropertyDestroy(meson_prop_HDCP);
            printf("\n prop value:%llu objID:%d,name:%s\n",value_3, objID,propName);
            if (value_3 == 1)
                *prop_value = 2;
        }
    }
    mesonPropertyDestroy(meson_prop);
    ret = 0;
out:
    if (conn)
        mesonConnectorDestroy(drmFd,conn);
    if (drmFd >= 0 )
        close(drmFd);
    return ret;
}

int meson_drm_open()
{
    int ret_fd = -1;
    const char *card;
    card= getenv("WESTEROS_DRM_CARD");
    if ( !card ) {
        card = DEFAULT_CARD;
    }
    ret_fd = open(card, O_RDONLY|O_CLOEXEC);
    if ( ret_fd < 0 )
        printf("\n drm card:%s open fail\n",card);
    else
        drmDropMaster(ret_fd);
    return ret_fd;
}
struct mesonConnector* get_current_connector(int drmFd)
{
    struct mesonConnector* connectorHDMI = NULL;
    struct mesonConnector* connectorLVDS = NULL;
    struct mesonConnector* connectorCVBS = NULL;
    int HDMIconnected = 0;
    int TVConnected = 0;
    connectorHDMI = mesonConnectorCreate(drmFd, DRM_MODE_CONNECTOR_HDMIA);
    if (connectorHDMI)
        HDMIconnected = mesonConnectorGetConnectState(connectorHDMI);
    if (HDMIconnected == 1) {
        return connectorHDMI;
    } else {
        connectorLVDS = mesonConnectorCreate(drmFd, DRM_MODE_CONNECTOR_LVDS);
        if (connectorLVDS)
            TVConnected = mesonConnectorGetConnectState(connectorLVDS);
        if (TVConnected == 1) {
            return connectorLVDS;
        } else {
            connectorCVBS = mesonConnectorCreate(drmFd, DRM_MODE_CONNECTOR_TV);
            return connectorCVBS;
        }
    }
}

void meson_drm_close_fd(int drmFd)
{
    if (drmFd >= 0)
        close(drmFd);
}

static int  _amsysfs_get_sysfs_str(const char *path, char *valstr, int size)
{
    int fd;
    fd = open(path, O_RDONLY);
    if (fd >= 0) {
        memset(valstr,0,size);
        read(fd, valstr, size - 1);
        valstr[strlen(valstr)] = '\0';
        close(fd);
    } else {
        sprintf(valstr, "%s", "fail");
        return -1;
    };
    return 0;
}

static int  _get_frac_rate_policy()
{
    int ret = 0;
    char buffer[10] = {'\0'};
    if (0 == _amsysfs_get_sysfs_str(FRAC_RATE_POLICY, buffer, sizeof(buffer)-1) ) {
        if (strstr(buffer, "1"))
            ret = 1;
    }
    return ret;
}

int meson_drm_get_vblank_time(int drmFd, int nextVsync,uint64_t *vblankTime, uint64_t *refreshInterval)
{
    int ret = -1;
    int rc = -1;
    struct mesonConnector* connector = NULL;
    drmModeModeInfo* mode = NULL;
    if (drmFd < 0) {
        printf("\n drmFd error\n");
        goto out;
    }
    if (nextVsync < 0)
        nextVsync = 0;
    connector = get_current_connector(drmFd);
    if (connector != NULL ) {
       mode = mesonConnectorGetCurMode(drmFd, connector);
       if (mode) {
           *refreshInterval = (1000000LL+(mode->vrefresh/2)) / mode->vrefresh;
           if ( ( mode->vrefresh == 60 || mode->vrefresh == 30 || mode->vrefresh == 24
                  || mode->vrefresh == 120 || mode->vrefresh == 240 )
               && _get_frac_rate_policy() == 1 ) {
              *refreshInterval = (1000000LL+(mode->vrefresh/2)) * 1001 / mode->vrefresh / 1000;
           }
           free(mode);
       }
    }
    drmVBlank vbl;
    vbl.request.type= DRM_VBLANK_RELATIVE;
    vbl.request.sequence= nextVsync;
    vbl.request.signal= 0;
    rc = drmWaitVBlank(drmFd, &vbl );
    if (rc != 0 ) {
        printf("drmWaitVBlank failed: rc %d errno %d",rc, errno);
        ret = -1;
        goto out;
    }
    if ((rc == 0) && (vbl.reply.tval_sec > 0 || vbl.reply.tval_usec > 0)) {
        *vblankTime = vbl.reply.tval_sec * 1000000LL + vbl.reply.tval_usec;
    }
    ret = 0;
out:
    mesonConnectorDestroy(drmFd, connector);
    return ret;
}


