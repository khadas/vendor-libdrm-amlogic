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
#include <linux/string.h>
#include "libdrm_meson_connector.h"
#include "libdrm_meson_property.h"
#include "meson_drm_display.h"


#define DEFAULT_CARD "/dev/dri/card0"
#ifndef XDG_RUNTIME_DIR
#define XDG_RUNTIME_DIR     "/run"
#endif
static int s_drm_fd = -1;
struct mesonConnector* s_conn_HDMI = NULL;

static int meson_drm_setprop(int obj_id, char* prop_name, int prop_value );
static bool meson_drm_init();
static void meson_drm_deinit();
static uint32_t _getHDRSupportedList(uint64_t hdrlist, uint64_t dvlist);

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
                    printf("meson_drm_setprop: failed !!\n");
                    break;
                }
                xdgRunDir = XDG_RUNTIME_DIR;
            }
        } while (ret != 0);
    }
    return ret;
}

static bool meson_drm_init()
{
    bool ret = false;
    const char *card;
    int drm_fd = -1;
    card= getenv("WESTEROS_DRM_CARD");
    if ( !card ) {
        card = DEFAULT_CARD;
    }
    s_drm_fd = open(card, O_RDONLY|O_CLOEXEC);
    if ( s_drm_fd < 0 ) {
        printf("\n drm card:%s open fail\n",card);
        ret = false;
        goto exit;
    }
    s_conn_HDMI = mesonConnectorCreate(s_drm_fd, DRM_MODE_CONNECTOR_HDMIA);
    if ( !s_conn_HDMI ) {
        printf("\n  create HDMI connector fail\n");
        ret = false;
        goto exit;
    }
    ret = true;
exit:
    return ret;
}

static void meson_drm_deinit()
{
    if (s_conn_HDMI)
        mesonConnectorDestroy(s_drm_fd,s_conn_HDMI);
    if (s_drm_fd >= 0 )
        close(s_drm_fd);
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

int meson_drm_setMode(char* mode)
{
    int ret = -1;
	char cmdBuf[512] = {'\0'};
	char output[64] = {'\0'};
    printf(" meson_drm_setMode %s\n",mode);
    char* xdgRunDir = getenv("XDG_RUNTIME_DIR");
    if (!xdgRunDir)
        xdgRunDir = XDG_RUNTIME_DIR;
    if (!mode) {
        ret = -1;
    } else {
        do {
            snprintf(cmdBuf, sizeof(cmdBuf)-1, "export XDG_RUNTIME_DIR=%s;westeros-gl-console set mode %s | grep \"Response\"",
                    xdgRunDir, mode);
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
                    printf("meson_drm_setprop: failed !!\n");
                    break;
                }
                xdgRunDir = XDG_RUNTIME_DIR;
            }
        } while (ret != 0);
    }
    return ret;
}
char* meson_drm_getMode()
{
    char cmdBuf[512] = {'\0'};
    char output[64] = {'\0'};
    char* mode = NULL;
    char* mode_name = NULL;
    char* tok = NULL;
    char* ctx;
    char* xdgRunDir = getenv("XDG_RUNTIME_DIR");
    int temp = -1;
    if (!xdgRunDir)
        xdgRunDir = XDG_RUNTIME_DIR;
    do {
        snprintf(cmdBuf, sizeof(cmdBuf)-1, "export XDG_RUNTIME_DIR=%s;westeros-gl-console get mode | grep \"Response\"",
                xdgRunDir);
        printf("Executing '%s'\n", cmdBuf);
        /* FIXME: popen in use */
        FILE* fp = popen(cmdBuf, "r");
        if (NULL != fp) {
            while (fgets(output, sizeof(output)-1, fp)) {
                if (strlen(output) && strstr(output, "[0:")) {
                    mode = strstr(output, "[0:");
                    printf("\n  meson_drm_getMode:%s\n", mode);
                    int len = strlen(mode);
                    mode_name = (char*)calloc(len, sizeof(char));
                    sscanf(mode, "[0: mode %s",mode_name);
                    tok= strtok_r( mode_name, "]", &ctx );
                    printf("\n  mode_name   :%s\n", tok);
                    temp = 0;
                }
            }
            pclose(fp);
        } else {
            printf(" popen failed\n");
        }
        if (temp != 0 ) {
                if (strcmp(xdgRunDir, XDG_RUNTIME_DIR) == 0) {
                    printf("meson_drm_setprop: failed !!\n");
                    break;
                }
                xdgRunDir = XDG_RUNTIME_DIR;
        }
    } while ( temp != 0 );
    return tok;
}

int meson_drm_getRxSurportedModes( DisplayMoode** modes, int* modeCount )
{
    int ret = -1;
    if (!meson_drm_init()) {
        printf("\n drm card open fail\n");
        goto out;
    }
    drmModeModeInfo* modeall = NULL;
    int count = 0;
    int i = 0;
    if (0 != mesonConnectorGetModes(s_conn_HDMI, s_drm_fd, &modeall, &count))
        goto out;
    DisplayMoode* modestemp =  (DisplayMoode*)calloc(count, sizeof(DisplayMoode));
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
    meson_drm_deinit();
    return ret;
}
int meson_drm_getRxPreferredMode( DisplayMoode* mode)
{
    int ret = -1;
    int i = 0;
    int count = 0;
    drmModeModeInfo* modes = NULL;
    if (!meson_drm_init()) {
        printf("\n drm card open fail\n");
        goto out;
    }
    if (0 != mesonConnectorGetModes(s_conn_HDMI, s_drm_fd, &modes, &count))
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
    meson_drm_deinit();
    return ret;
}

int meson_drm_getEDID( int * data_Len, char **data)
{
    int ret = -1;
    int i = 0;
    int count = 0;
    drmModeModeInfo* modes = NULL;
    char* edid_data = NULL;
    if (!meson_drm_init()) {
        printf("\n drm card open fail\n");
        goto out;
    }
    if (0 != mesonConnectorGetEdidBlob(s_conn_HDMI, &count, &edid_data))
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
    meson_drm_deinit();
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
    if (meson_drm_init()) {
        int ConnectState = -1;
        ConnectState = mesonConnectorGetConnectState(s_conn_HDMI);
        if (ConnectState == 1) {
            ret = MESON_DRM_CONNECTED;
        } else if (ConnectState == 2) {
            ret = MESON_DRM_DISCONNECTED;
        } else {
            ret = MESON_DRM_UNKNOWNCONNECTION;
            meson_drm_deinit();
        }
    } else {
        printf("\n drm open fail\n");
    }
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
    if (!meson_drm_init()) {
        printf("\n drm card open fail\n");
        goto out;
    }
    switch (enProp)
    {
        case ENUM_DRM_PROP_HDMI_ENABLE:
        {
            objID =  mesonConnectorGetId(s_conn_HDMI);
            sprintf( propName, "%s", MESON_DRM_HDMITX_PROP_AVMUTE);
            prop_value = prop_value ? 0:1;
            break;
        }
        case ENUM_DRM_PROP_HDMITX_EOTF:
        {
            objID =  mesonConnectorGetCRTCId(s_conn_HDMI);
            sprintf( propName, "%s", MESON_DRM_HDMITX_PROP_EOTF);
            break;
        }
        case ENUM_DRM_PROP_CONTENT_PROTECTION:
        {
            objID =  mesonConnectorGetId(s_conn_HDMI);
            sprintf( propName, "%s", DRM_CONNECTOR_PROP_CONTENT_PROTECTION);
            break;
        }
        case ENUM_DRM_PROP_HDCP_VERSION:
        {
            objID =  mesonConnectorGetId(s_conn_HDMI);
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
            objID =  mesonConnectorGetCRTCId(s_conn_HDMI);
            sprintf( propName, "%s", DRM_CONNECTOR_PROP_TX_HDR_POLICY);
            break;
        }
        case ENUM_DRM_PROP_HDMI_ASPECT_RATIO:
        {
            objID =  mesonConnectorGetId(s_conn_HDMI);
            sprintf( propName, "%s", DRM_CONNECTOR_PROP_TX_ASPECT_RATIO);
            break;
        }
        default:
            break;
    }
    meson_drm_setprop(objID, propName, prop_value);
    if (enProp == ENUM_DRM_PROP_HDCP_VERSION)
    {
        objID =  mesonConnectorGetId(s_conn_HDMI);
        sprintf( propName, "%s", DRM_CONNECTOR_PROP_HDCP_PRIORITY);
        int priority = 0;
        if (force1_4)
        {
            priority = 1;
        }
        meson_drm_setprop(objID, propName, priority);
    }
    meson_drm_deinit();
    ret = 0;
out:
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
    if (!meson_drm_init()) {
        printf("\n drm card open fail\n");
        goto out;
    }
    switch (enProp)
    {
        case ENUM_DRM_PROP_HDMI_ENABLE:
        {
            objID =  mesonConnectorGetId(s_conn_HDMI);
            objtype = DRM_MODE_OBJECT_CONNECTOR;
            sprintf( propName, "%s", MESON_DRM_HDMITX_PROP_AVMUTE);
            break;
        }
        case ENUM_DRM_PROP_HDMITX_EOTF:
        {
            objID =  mesonConnectorGetCRTCId(s_conn_HDMI);
            objtype = DRM_MODE_OBJECT_CRTC;
            sprintf( propName, "%s", MESON_DRM_HDMITX_PROP_EOTF);
            break;
        }
        case ENUM_DRM_PROP_CONTENT_PROTECTION:
        {
            objID =  mesonConnectorGetId(s_conn_HDMI);
            objtype = DRM_MODE_OBJECT_CONNECTOR;
            sprintf( propName, "%s", DRM_CONNECTOR_PROP_CONTENT_PROTECTION);
            break;
        }
        case ENUM_DRM_PROP_HDCP_VERSION:
        {
            objID =  mesonConnectorGetId(s_conn_HDMI);
            objtype = DRM_MODE_OBJECT_CONNECTOR;
            sprintf( propName, "%s", DRM_CONNECTOR_PROP_CONTENT_TYPE);
            break;
        }
        case ENUM_DRM_PROP_HDR_POLICY:
        {
            objID =  mesonConnectorGetCRTCId(s_conn_HDMI);
            sprintf( propName, "%s", DRM_CONNECTOR_PROP_TX_HDR_POLICY);
            objtype = DRM_MODE_OBJECT_CRTC;
            break;
        }
        case ENUM_DRM_PROP_GETRX_HDCP_SUPPORTED_VERS:
        {
            objID =  mesonConnectorGetId(s_conn_HDMI);
            objtype = DRM_MODE_OBJECT_CONNECTOR;
            sprintf( propName, "%s", DRM_CONNECTOR_PROP_RX_HDCP_SUPPORTED_VER);
            break;
        }
        case ENUM_DRM_PROP_GETRX_HDR_CAP:
        {
            objID =  mesonConnectorGetId(s_conn_HDMI);
            objtype = DRM_MODE_OBJECT_CONNECTOR;
            sprintf( propName, "%s", DRM_CONNECTOR_PROP_RX_HDR_CAP);
            break;
        }
        case ENUM_DRM_PROP_GETTX_HDR_MODE:
        {
            objID =  mesonConnectorGetId(s_conn_HDMI);
            objtype = DRM_MODE_OBJECT_CONNECTOR;
            sprintf( propName, "%s", DRM_CONNECTOR_PROP_TX_HDR_MODE);
            break;
        }
        case ENUM_DRM_PROP_HDMI_ASPECT_RATIO:
        {
            objID =  mesonConnectorGetId(s_conn_HDMI);
            objtype = DRM_MODE_OBJECT_CONNECTOR;
            sprintf( propName, "%s", DRM_CONNECTOR_PROP_TX_ASPECT_RATIO);
            break;
        }
        default:
            break;
    }
    struct mesonProperty* meson_prop = NULL;
    meson_prop = mesonPropertyCreate(s_drm_fd, objID, objtype, propName);
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
        objID =  mesonConnectorGetId(s_conn_HDMI);
        objtype = DRM_MODE_OBJECT_CONNECTOR;
        sprintf( propName, "%s", DRM_CONNECTOR_PROP_RX_DV_CAP);
        struct mesonProperty* meson_prop_dv = NULL;
        meson_prop_dv = mesonPropertyCreate(s_drm_fd, objID, objtype, propName);
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
            objID =  mesonConnectorGetId(s_conn_HDMI);
            objtype = DRM_MODE_OBJECT_CONNECTOR;
            sprintf( propName, "%s", DRM_CONNECTOR_PROP_HDCP_PRIORITY);
            struct mesonProperty* meson_prop_HDCP = NULL;
            meson_prop_HDCP = mesonPropertyCreate(s_drm_fd, objID, objtype, propName);
            uint64_t value_3 = mesonPropertyGetValue(meson_prop_HDCP);
            mesonPropertyDestroy(meson_prop_HDCP);
            printf("\n prop value:%llu objID:%d,name:%s\n",value_3, objID,propName);
            if (value_3 == 1)
                *prop_value = 2;
        }
    }
    mesonPropertyDestroy(meson_prop);
    meson_drm_deinit();
    ret = 0;
out:
    return ret;
}



