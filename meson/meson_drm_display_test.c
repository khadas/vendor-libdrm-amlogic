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
#include "meson_drm_event.h"

void display_event( ENUM_DISPLAY_EVENT enEvent, void *eventData/*Optional*/)
{
    printf("\n DISPLAY_EVENT_CONNECTED = 0,//!< Display connected event.\n"
               "DISPLAY_EVENT_DISCONNECTED//!< Display disconnected event.\n"
               "DISPLAY_HDCP_AUTHENTICATED,          //!< HDCP authenticate success\n"
               "DISPLAY_HDCP_AUTHENTICATIONFAILURE \n"
               "Eevent:%d\n",enEvent);
}
int main(void )
{
    printf("\n test meson_drm_display_test start\n");
    int select_s_g = 0;
    uint32_t value = 255;
    printf("\n 0->set prop 1->get  2->HPD/HDCP event test\n");
    scanf("%d",&select_s_g);
    if (select_s_g == 1) {
        printf("get value:1->HDMI enable 2->mode 3->EOTF value 4->Content Protection 5->HDCP version 6->HDR_POLICY 7->HDMI connected 8->Rx supported modes 9->prefer mode 10->edid\n"
               "11->Rx supported HDCP versions 12->Rx supported HDR modes 13->tx Cur HDR mode 14->HDCP auth mode 15->get current aspect ratio value 16->dv enable 17->vblank time\n ");
        int get = 0;
        scanf("%d", &get);
        if (get == 1) {
            meson_drm_get_prop(ENUM_DRM_PROP_HDMI_ENABLE, &value);
            printf("\n get HDMI enable:%d\n", value);
        } else if (get == 2) {
            DisplayMode mode;
            if (meson_drm_getMode(&mode) == 0) {
                printf("\n mode (%d %d %d %d)\n",mode.w, mode.h, mode.vrefresh, mode.interlace);
            }
        } else if (get == 3) {
            meson_drm_get_prop(ENUM_DRM_PROP_HDMITX_EOTF, &value);
            printf("\n EOTF:%d\n",value);
        } else if (get == 4) {
            meson_drm_get_prop(ENUM_DRM_PROP_CONTENT_PROTECTION, &value);
            printf("\n Content Protection:%d\n", value);
        } else if (get == 5) {
            meson_drm_get_prop(ENUM_DRM_PROP_HDCP_VERSION, &value);
            printf("\n HDMI version:%d\n",value);
        }
        else if (get == 6) {
            meson_drm_get_prop(ENUM_DRM_PROP_HDR_POLICY, &value);
            printf("\n HDR_POLICY:%d\n", value);
        } else if (get == 7) {
            printf("\n HDMI connected:%d\n",meson_drm_getConnection());
        } else if (get == 8) {
            DisplayMode* modes = NULL;
            int count = 0;
            if (0 == meson_drm_getRxSurportedModes( &modes, &count )) {
                printf("\n mode count:%d\n",count);
                int i = 0;
                for (int i=0; i<count; i++) {
                    printf(" (%s %d %d %d %d)\n", modes[i].name, modes[i].w, modes[i].h, modes[i].interlace,modes[i].vrefresh);
                }
                if (modes)
                    free(modes);
            } else {
                 printf("\n %s fail\n",__FUNCTION__);
            }
        } else if (get == 9) {
            DisplayMode mode;
            if (0 == meson_drm_getRxPreferredMode(&mode)) {
                printf(" (%s %d %d %d)\n", mode.name, mode.w, mode.h, mode.interlace);
            } else {
                 printf("\n %s fail\n",__FUNCTION__);
            }
        } else if (get == 10) {
            int count = 0;
            char* edid = NULL;
            if ( 0 == meson_drm_getEDID(&count, &edid)) {
                int i = 0;
                for (i=0; i<count; i++) {
                    if (i%16 == 0)
                        printf("\n");
                    printf("%.2hhx ",edid[i]);
                }
                printf("\n");
                if (edid)
                    free(edid);
            }
        }
        else if (get == 11) {
            if (0 == meson_drm_get_prop( ENUM_DRM_PROP_GETRX_HDCP_SUPPORTED_VERS, &value)) {
                if (value & 0x1)
                    printf("\n RX HDCP 1.4 supported\n");
                if (value & 0x2)
                    printf("\n RX HDCP 2.2 supported\n");
            } else {
                printf("\n meson_drm_get_prop fail\n");
            }
        }
        else if (get == 12) {
            if (0 == meson_drm_get_prop(ENUM_DRM_PROP_GETRX_HDR_CAP, &value)) {
                if (value & 0x1)
                    printf("\n MESON_DRM_HDR10PLUS\n");
                if (value & 0x2)
                    printf("\n MESON_DRM_DOLBYVISION_STD\n");
                if (value & 0x4)
                    printf("\n MESON_DRM_DOLBYVISION_LL\n");
                if (value & 0x8)
                    printf("\n MESON_DRM_HDR10_ST2084\n");
                if (value & 0x10)
                    printf("\n MESON_DRM_HDR10_TRADITIONAL\n");
                if (value & 0x20)
                    printf("\n MESON_DRM_HDR_HLG\n");
                if (value & 0x40)
                    printf("\n MESON_DRM_SDR\n");
             } else {
                printf("\n meson_drm_get_prop fail\n");
             }
        } else if (get == 13) {
            if (0 == meson_drm_get_prop( ENUM_DRM_PROP_GETTX_HDR_MODE, &value)) {
                printf("\n cur hdr mode:%d\n",value);
            } else {
                 printf("\n meson_drm_get_prop fail\n");
            }
        }
        else if (get == 14) {
            if (0 == meson_drm_get_prop( ENUM_DRM_PROP_GETRX_HDCP_AUTHMODE, &value)) {
                if (value & 0x1)
                    printf("\n HDCP 14\n");
                if (value & 0x2)
                    printf("\n HDCP 22\n");
                if (value & 0x8)
                    printf("\n success\n");
                else
                    printf("\n fail\n");
             } else {
                    printf("\n meson_drm_get_prop fail\n");
             }
        }
        else if (get == 15) {
           if (0 == meson_drm_get_prop( ENUM_DRM_PROP_HDMI_ASPECT_RATIO, &value)) {
                if (value == 0) {
                    printf("\n current mode do not support aspect ratio change\n");
                } else if (value == 1) {
                    printf("\n current aspect ratio is 4:3 and you can switch to 16:9\n");
                } else if (value == 2) {
                    printf("\n current aspect ratio is 16:9 and you can switch to 4:3\n");
                } else {
                     printf("\n invalid value\n");
                }
            } else {
                printf("\n meson_drm_get_prop fail\n");
            }
        } else if (get == 16) {
            if (0 == meson_drm_get_prop( ENUM_DRM_PROP_HDMI_DV_ENABLE, &value)) {
                printf("\n meson_drm_get_prop DV enable:%d\n",value);
            } else {
                printf("\n meson_drm_get_prop fail\n");
            }
        } else if (get == 17) {
            uint64_t vlankTime = 0;
            uint64_t refreshInterval = 0;
            int fd = meson_drm_open();
            int nextVsync = 0;
            printf("\n please input nextvsync value:\n");
            scanf("%d", &nextVsync);
            if (0 == meson_drm_get_vblank_time(fd, nextVsync, &vlankTime, &refreshInterval))
                printf("\n meson_drm_get_prop vlankTime:%llu, refreshInterval:%llu\n",vlankTime, refreshInterval);
            else
                printf("\n meson_get_vblank_time fail\n");
            meson_drm_close_fd(fd);
        }
        else {
            printf("\n invalid input\n");
        }
    } else if (select_s_g == 0) {
        printf("get value:1->HDMI enable 2->mode 3->EOTF value 4->Content Protection 5->HDCP Version 6->HDR_POLICY 7->aspect ratio 8->dv enable\n");
        int set = 0;
        scanf("%d", &set);
        if (set == 1) {
            printf("\n 0->disable HDMI AV. 1->enable HDMI AV\n");
            int enable = 1;
            scanf("%d",&enable);
            if (meson_drm_set_prop(ENUM_DRM_PROP_HDMI_ENABLE, enable))
                printf("\n set HDMI enable success\n");
        } else if (set == 2) {
            printf("\n set video format\n");

            int w=0, h=0,refresh=0,interlace=0;
            scanf("%d",&w);
            scanf("%d",&h);
            scanf("%d",&refresh);
            scanf("%d",&interlace);
            DisplayMode mode;
            mode.w = w;
            mode.h = h;
            mode.vrefresh = refresh;
            mode.interlace = interlace;
            if (meson_drm_setMode( &mode ) == 0)
                printf("\n set Video Format success\n");

        } else if (set == 3) {
            printf("\n HDMI_EOTF_TRADITIONAL_GAMMA_SDR = 0 \n"
                     " HDMI_EOTF_SMPTE_ST20842 = 2 \n"
                     " HDMI_EOTF_NODV_HDR_SDR = 18 \n"
                     " HDMI_EOTF_NODV_SDR_HDR = 19 \n"
                );
            int hdr = 1;
            scanf("%d",&hdr);
            if (meson_drm_set_prop(ENUM_DRM_PROP_HDMITX_EOTF, hdr))
                printf("\n set HDMI enable success\n");

        } else if (set == 4) {
            printf("\n 0->disable content protection. 1->enable content protection\n");
            int enable = 1;
            scanf("%d",&enable);
            if (meson_drm_set_prop(ENUM_DRM_PROP_CONTENT_PROTECTION, enable))
                printf("\n set HDMI enable success\n");

        } else if (set == 5) {
            printf("\n version: ENUM_HDCP_VERSION_DEFAULT = 0,"
                   "\n ENUM_HDCP_VERSION_FORCE_2_2,"
                   "\n ENUM_HDCP_VERSION_FORCE_1_4,\n");
            int version = 1;
            scanf("%d",&version);
             if (meson_drm_set_prop(ENUM_DRM_PROP_HDCP_VERSION, version))
                printf("\n set HDMI enable success\n");
        } else if (set == 6) {
            printf("\n HDR_POLICY:\n");
            int policy =-1;
            scanf("%d",&policy);
             if (meson_drm_set_prop(ENUM_DRM_PROP_HDR_POLICY, policy))
                printf("\n set HDMI enable success\n");
        }
        else if (set == 7) {
            printf("\n aspect ratio:\n");
            int ar =-1;
            scanf("%d",&ar);
            if (0 == meson_drm_get_prop( ENUM_DRM_PROP_HDMI_ASPECT_RATIO, &value)) {
                if (value == 0) {
                    printf("\n current mode do not support aspect ratio change\n");
                } else {
                    if (ar == 1 && value == 2) {
                        if (0 == meson_drm_set_prop(ENUM_DRM_PROP_HDMI_ASPECT_RATIO, ar))
                            printf("\n aspect ratio 4:3 set success\n");
                    } else if (ar == 2 && value == 1) {
                        if (0 == meson_drm_set_prop(ENUM_DRM_PROP_HDMI_ASPECT_RATIO, ar))
                            printf("\n aspect ratio 16:9 set success\n");
                    } else {
                        printf("\n aspect ratio invalid\n");
                    }
                }
            } else {
                printf("\n get current aspect ratio value fail\n");
            }
        } else if (set == 8) {
            printf("\n DV enable:\n");
            int enable =-1;
            scanf("%d",&enable);
             if (meson_drm_set_prop(ENUM_DRM_PROP_HDMI_DV_ENABLE, enable))
                printf("\n set HDMI DV enable success\n");
        }
    }
    else {
        RegisterDisplayEventCallback(display_event);
        startDisplayUeventMonitor();
        while (1)
        {
            usleep(20000);
        }
    }
    getchar();
exit:
    return 0;
}

