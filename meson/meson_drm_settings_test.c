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
#include "meson_drm_settings.h"

int main(void )
{
    printf("\n test meson_drm_settings test start\n");
    int select_s_g = 0;
    uint32_t value = 255;
    printf("\n 0->set prop 1->get\n");
    scanf("%d",&select_s_g);
    if (select_s_g == 1) {
        printf("get value:1.HDCP version 2.HDMI connected 3.color space 4. color depth 5. hdr mode 6. mode 7. hdr policy\n");
        int get = 0;
        int drmFd = meson_open_drm();
        int len = scanf("%d", &get);
        if (get == 1) {
            ENUM_MESON_HDCP_VERSION value = meson_drm_getHdcpVersion( drmFd,MESON_CONNECTOR_HDMIA );
            printf("\n MESON_HDCP_14      = 0\n"
                     " MESON_HDCP_22      = 1\n value:%d \n", value);
        } else if (get == 2) {
            ENUM_MESON_CONN_CONNECTION value = meson_drm_getConnectionStatus(drmFd,MESON_CONNECTOR_HDMIA);
            printf("\n MESON_DISCONNECTED      = 0\n"
                     " MESON_CONNECTED         = 1\n value:%d \n",value);
        } else if (get == 3) {
            ENUM_MESON_COLOR_SPACE value = meson_drm_getColorSpace( drmFd, MESON_CONNECTOR_HDMIA );
            printf("\n MESON_COLOR_SPACE_RGB      = 0 \n"
                      "MESON_COLOR_SPACE_YCBCR422 = 1 \n"
                      "MESON_COLOR_SPACE_YCBCR444 = 2 \n"
                      "MESON_COLOR_SPACE_YCBCR420 = 3 \n value:%d\n"
                      , value);
        } else if (get == 4) {
            uint32_t value = meson_drm_getColorDepth( drmFd, MESON_CONNECTOR_HDMIA );
            printf("\n color depth::%d\n",value);
        }
        else if (get == 5) {
            ENUM_MESON_HDR_MODE value = meson_drm_getHdrStatus( drmFd, MESON_CONNECTOR_HDMIA );
            printf("\n MESON_HDR10PLUS      = 0 \n"
                     " MESON_DOLBYVISION_STD    \n"
                     " MESON_DOLBYVISION_LL    \n"
                     " MESON_HDR10_ST2084    \n"
                     " MESON_HDR10_TRADITIONAL    \n"
                     " MESON_HDR_HLG    \n"
                     " MESON_SDR    \n value:%d\n"
                     , value);
        } else if (get == 6) {
            DisplayMode mode;
            if (meson_drm_getModeInfo(drmFd, MESON_CONNECTOR_HDMIA, &mode ) == 0) {
                printf("\n mode (%d %d %d %d)\n",mode.w, mode.h, mode.vrefresh, mode.interlace);
            }
        }  else if (get == 7) {
            ENUM_MESON_HDR_POLICY value = meson_drm_getHDRPolicy(drmFd, MESON_CONNECTOR_HDMIA );
            printf("\n MESON_HDR_POLICY_FOLLOW_SINK      = 0 \n"
                      "MESON_HDR_POLICY_FOLLOW_SOURCE = 1 \n value:%d\n", value);
        }
        meson_close_drm(drmFd);
    } else if (select_s_g == 0) {
        printf("\n set prop test not implement\n");
    }
    getchar();
exit:
    return 0;
}

