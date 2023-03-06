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
#include <linux/string.h>
#include "xf86drm.h"
#include "xf86drmMode.h"
#include "libdrm_meson_connector.h"
#include "libdrm_meson_property.h"
#include "meson_drm_settings.h"
#define DEFAULT_CARD "/dev/dri/card0"

int main(void )
{
    printf("\n test meson_drm_settings test start\n");
    int select_s_g = 0;
    uint32_t value = 255;
    int select_len = 0;
    printf("\n 0->set prop 1->get\n");
    select_len = scanf("%d",&select_s_g);
    if (select_s_g == 1 && select_len == 1) {
        printf("get value: 1.HDCP version  2.HDMI connected 3.color space 4. color depth"
               "5. hdr mode 6. mode 7. hdr policy 8. EDID 9. hdcp auth status\n");
        int get = 0;
        int drmFd = meson_open_drm();
        int len = scanf("%d", &get);
        if (get == 1 && len == 1) {
            ENUM_MESON_HDCP_VERSION value = meson_drm_getHdcpVersion( drmFd,MESON_CONNECTOR_HDMIA );
            printf("\n MESON_HDCP_14      = 0\n"
                     " MESON_HDCP_22      = 1\n value:%d \n", value);
        } else if (get == 2 && len == 1) {
            ENUM_MESON_CONN_CONNECTION value = meson_drm_getConnectionStatus(drmFd,MESON_CONNECTOR_HDMIA);
            printf("\n MESON_DISCONNECTED      = 0\n"
                     " MESON_CONNECTED         = 1\n value:%d \n",value);
        } else if (get == 3 && len == 1) {
            ENUM_MESON_COLOR_SPACE value = meson_drm_getColorSpace( drmFd, MESON_CONNECTOR_HDMIA );
            printf("\n MESON_COLOR_SPACE_RGB      = 0 \n"
                      "MESON_COLOR_SPACE_YCBCR422 = 1 \n"
                      "MESON_COLOR_SPACE_YCBCR444 = 2 \n"
                      "MESON_COLOR_SPACE_YCBCR420 = 3 \n value:%d\n"
                      , value);
        } else if (get == 4 && len == 1) {
            uint32_t value = meson_drm_getColorDepth( drmFd, MESON_CONNECTOR_HDMIA );
            printf("\n color depth:%d\n",value);
        }
        else if (get == 5 && len == 1) {
            ENUM_MESON_HDR_MODE value = meson_drm_getHdrStatus( drmFd, MESON_CONNECTOR_HDMIA );
            printf("\n MESON_HDR10PLUS      = 0 \n"
                     " MESON_DOLBYVISION_STD    \n"
                     " MESON_DOLBYVISION_LL    \n"
                     " MESON_HDR10_ST2084    \n"
                     " MESON_HDR10_TRADITIONAL    \n"
                     " MESON_HDR_HLG    \n"
                     " MESON_SDR    \n value:%d\n"
                     , value);
        } else if (get == 6 && len == 1) {
            DisplayMode mode;
            if (meson_drm_getModeInfo(drmFd, MESON_CONNECTOR_HDMIA, &mode ) == 0) {
                printf("\n mode (%d %d %d %d)\n",mode.w, mode.h, mode.vrefresh, mode.interlace);
            }
        }  else if (get == 7 && len == 1) {
            ENUM_MESON_HDR_POLICY value = meson_drm_getHDRPolicy(drmFd, MESON_CONNECTOR_HDMIA );
            printf("\n MESON_HDR_POLICY_FOLLOW_SINK      = 0 \n"
                      "MESON_HDR_POLICY_FOLLOW_SOURCE = 1 \n value:%d\n", value);
        } else if (get == 8 && len == 1) {
            int len = 0;
            char *edid = NULL;
            int i;
            meson_drm_getEDIDData(drmFd, MESON_CONNECTOR_HDMIA, &len, &edid );
            printf("\n EDID data len:%d\n", len);
            for (i = 0; i < len; i++) {
                if (i % 16 == 0)
                    printf("\n\t\t\t");
                if (edid)
                    printf("%.2hhx", edid[i]);
            }
            printf("\n");
            if (edid)
                free(edid);
        } else if (get == 9 && len == 1) {
            ENUM_MESON_HDCPAUTH_STATUS value = MESON_AUTH_STATUS_FAIL;
            value = meson_drm_getHdcpAuthStatus( drmFd, MESON_CONNECTOR_HDMIA );
             printf("\n MESON_AUTH_STATUS_FAIL      = 0 \n"
                      " MESON_AUTH_STATUS_SUCCESS = 1 \n value:%d\n", value);
        }
        meson_close_drm(drmFd);
    } else if (select_s_g == 0 && select_len == 1) {
        printf("set value:1.av mute 2.HDMI HDCP enable \n");
        int set = 0;
        int ret = -1;
        drmModeAtomicReq * req;
        int drmFd = open(DEFAULT_CARD, O_RDWR|O_CLOEXEC);
        if (drmFd < 0) {
            fprintf(stderr, "failed to open device %s\n", strerror(errno));
        }
        /*use atomic*/
        ret =  drmSetClientCap(drmFd, DRM_CLIENT_CAP_ATOMIC, 1);
        if (ret) {
            fprintf(stderr, "no atomic modesetting support: %s\n", strerror(errno));
            drmClose(drmFd);
        }
        req = drmModeAtomicAlloc();
        int len = scanf("%d", &set);
        if (set == 1 && len == 1) {
            printf("\n av mute:\n");
            int avmute = 0;
            len = scanf("%d", &avmute);
            if (len == 1) {
                if (meson_drm_setAVMute(drmFd, req, avmute, MESON_CONNECTOR_HDMIA))
                    printf("\n meson_drm_setAVMute fail:\n");
            } else {
                printf("\n scanf fail\n");
            }
        } else if (set == 2 && len == 1) {
            printf("\n HDCP enable:\n");
            int hdcpEnable = 0;
            len = scanf("%d", &hdcpEnable);
            if (len == 1) {
                if (meson_drm_setHDCPEnable( drmFd, req, hdcpEnable, MESON_CONNECTOR_HDMIA))
                    printf("\n meson_drm_setAVMute fail:\n");
                } else {
                    printf("\n scanf fail\n");
                }
        }
        ret = drmModeAtomicCommit(drmFd, req, DRM_MODE_ATOMIC_ALLOW_MODESET, NULL);
        if (ret) {
            fprintf(stderr, "failed to set mode: %d-%s\n", ret, strerror(errno));
        }
        drmModeAtomicFree(req);
        drmClose(drmFd);
    }
    getchar();
exit:
    return 0;
}

