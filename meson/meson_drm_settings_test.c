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
               "5. hdr mode 6. mode 7. hdr policy 8. EDID 9. hdcp auth status 10.supportedModesList"
               " 11.prefer mode 12.HDCP Content Type 13.Content Type 14.Dv Enable 15.active "
               " 16.vrr Enable 17.AVMute 18.Hdrcap 19.DvCap 20.default modeInfo 21.current aspect ratio value"
               " 22.frac rate policy 23.hdr force mode 24.dpms status 25.plane size 26.physical size\n");
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
            ENUM_MESON_HDR_MODE value = meson_drm_getHdrStatus(drmFd, MESON_CONNECTOR_HDMIA );
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
                      "MESON_HDR_POLICY_FOLLOW_SOURCE = 1 \n"
                      "MESON_HDR_POLICY_FOLLOW_FORCE_MODE = 2 \n value:%d\n", value);
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
        }  else if (get == 10 && len == 1) {
            DisplayMode* modes = NULL;
            int count = 0;
            if (0 == meson_drm_getsupportedModesList(drmFd, &modes, &count ,MESON_CONNECTOR_HDMIA)) {
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
        } else if (get == 11 && len == 1) {
            DisplayMode mode;
            if (0 == meson_drm_getPreferredMode(&mode,MESON_CONNECTOR_HDMIA)) {
                printf(" (%s %d %d %d)\n", mode.name, mode.w, mode.h, mode.interlace);
            } else {
                 printf("\n %s fail\n",__FUNCTION__);
            }
        } else if (get == 12 && len == 1) {
            ENUM_MESON_HDCP_Content_Type value = MESON_HDCP_Type_RESERVED;
            value = meson_drm_getHDCPContentType(drmFd, MESON_CONNECTOR_HDMIA);
            printf("\n MESON_HDCP_Type0      = 0 \n"
                      " MESON_HDCP_Type1 = 1 \n value:%d\n", value);
        } else if (get == 13 && len == 1) {
            ENUM_MESON_COLOR_SPACE value = meson_drm_getContentType( drmFd, MESON_CONNECTOR_HDMIA );
            printf("\n MESON_Content_Type_Data      = 0 \n"
                      "MESON_Content_Type_Graphics = 1 \n"
                      "MESON_Content_Type_Photo = 2 \n"
                      "MESON_Content_Type_Cinema = 3 \n"
                      "MESON_Content_Type_Game = 4 \n value:%d\n"
                      , value);
        } else if (get == 14 && len == 1) {
            int value = meson_drm_getDvEnable( drmFd, MESON_CONNECTOR_HDMIA );
            printf("\n Dv_Enable:%d\n",value);
            if (value == 1) {
                printf("Support Dolbyvision\n");
            } else {
                printf("Dolbyvision not supported\n");
            }
        } else if (get == 15 && len == 1) {
            int value = meson_drm_getActive( drmFd, MESON_CONNECTOR_HDMIA );
            printf("\n Active:%d\n",value);
        } else if (get == 16 && len == 1) {
            int value = meson_drm_getVrrEnabled( drmFd, MESON_CONNECTOR_HDMIA );
            printf("\n Vrr_Enabled:%d\n",value);
        } else if (get == 17 && len == 1) {
            int value = meson_drm_getAVMute( drmFd, MESON_CONNECTOR_HDMIA );
            printf("\n AVMute:%d\n",value);
        }  else if (get == 18 && len == 1) {
            // presents the RX HDR capability
            uint32_t value = meson_drm_getHdrCap( drmFd, MESON_CONNECTOR_HDMIA );
            printf("\n hdrcap:%d\n",value);
        } else if (get == 19 && len == 1) {
            // presents the RX dolbyvision capability, [r] such as std or ll mode
            int value = meson_drm_getDvCap( drmFd, MESON_CONNECTOR_HDMIA );
            printf("\n DvCap:%d\n",value);
        } else if (get == 20 && len == 1) {
            DisplayMode mode;
            if (meson_drm_getModeInfo(drmFd, MESON_CONNECTOR_RESERVED, &mode ) == 0) {
                printf("\n mode (%d %d %d %d)\n",mode.w, mode.h, mode.vrefresh, mode.interlace);
            }
        } else if (get == 21  && len == 1) {
            int value = meson_drm_getAspectRatioValue( drmFd, MESON_CONNECTOR_HDMIA);
            if (value == 0) {
                printf("\n current mode do not support aspect ratio change\n"); //automatic
            } else if (value == 1) {
                printf("\n current aspect ratio is 4:3 and you can switch to 16:9\n");
            } else if (value == 2) {
                printf("\n current aspect ratio is 16:9 and you can switch to 4:3\n");
            } else {
                printf("\n invalid value\n");
            }
        } else if (get == 22 && len == 1) {
            int value = meson_drm_getFracRatePolicy( drmFd, MESON_CONNECTOR_HDMIA );
            if (value == -1) {
                printf("\n invalid value\n");
            } else {
                printf("\n FracRate: %d\n",value);
            }
        } else if (get == 23 && len == 1) {
            int value = meson_drm_getHdrForceMode( drmFd, MESON_CONNECTOR_HDMIA );
            printf("\n MESON_DRM_UNKNOWN_FMT      = 0 \n"
                      "MESON_DRM_BT709 = 1 \n"
                      "MESON_DRM_BT2020 = 2 \n"
                      "MESON_DRM_BT2020_PQ = 3 \n"
                     "MESON_DRM_BT2020_PQ_DYNAMIC = 4 \n"
                     "MESON_DRM_BT2020_HLG = 5 \n"
                     "MESON_DRM_BT2100_IPT = 6 \n"
                     "MESON_DRM_BT2020YUV_BT2020RGB_CUVA = 7 \n"
                     "MESON_DRM_BT_BYPASS = 8 \n value:%d\n"
                      , value);
        } else if (get == 24 && len == 1) {
            uint32_t value = meson_drm_getDpmsStatus( drmFd, MESON_CONNECTOR_HDMIA );
            printf("\n get dpms status: %d\n",value);
        } else if (get == 25 && len == 1) {
           int height = 0;
           int width = 0;
           int ret = meson_drm_getGraphicPlaneSize( drmFd, &width, &height);
           if (ret == 0 ) {
               printf("\n get graphic plane Size width = %d, height = %d\n", width, height);
           } else {
               printf("\n meson_drm_getGraphicPlaneSize fail\n");
           }
        } else if (get == 26 && len == 1) {
           uint32_t height = 0;
           uint32_t width = 0;
           int ret = meson_drm_getPhysicalSize( drmFd, &width, &height, MESON_CONNECTOR_HDMIA );
           if (ret == 0 ) {
               printf("\n get physical Size width = %d, height = %d\n", width, height);
           } else {
               printf("\n meson_drm_getPhysicalSize fail\n");
           }
        }
        meson_close_drm(drmFd);
    } else if (select_s_g == 0 && select_len == 1) {
        printf("set value:1.av mute 2.HDMI HDCP enable  3.HDCP Content Type "
        " 4.DvEnable 5.active 6.vrr Enable 7.video zorder 8.plane mute 9.aspect ratio"
        " 10.frac rate policy 11.hdr force mode 12.dv mode\n");
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
                    printf("\n meson_drm_setHDCPEnable fail:\n");
                } else {
                    printf("\n scanf fail\n");
                }
        } else if (set == 3 && len == 1) {
            printf("\n HDCP Content Type:\n");
            int HDCPContentType = 0;
            len = scanf("%d", &HDCPContentType);
            if (len == 1) {
                if (meson_drm_setHDCPContentType( drmFd, req, HDCPContentType, MESON_CONNECTOR_HDMIA))
                    printf("\n meson_drm_setHDCPContentType fail:\n");
                } else {
                    printf("\n scanf fail\n");
                }
        } else if (set == 4 && len == 1) {
            printf("\n DvEnable:\n");
            int dvEnable = 0;
            len = scanf("%d", &dvEnable);
            if (len == 1) {
                if (meson_drm_setDvEnable( drmFd, req, dvEnable, MESON_CONNECTOR_HDMIA))
                    printf("\n meson_drm_setDv_Enable fail:\n");
                } else {
                    printf("\n scanf fail\n");
                }
        } else if (set == 5 && len == 1) {
            printf("\n Active:\n");
            int active = 0;
            len = scanf("%d", &active);
            if (len == 1) {
                if (meson_drm_setActive( drmFd, req, active, MESON_CONNECTOR_HDMIA))
                    printf("\n meson_drm_setActive fail:\n");
                } else {
                    printf("\n scanf fail\n");
                }
        } else if (set == 6 && len == 1) {
            printf("\n vrr Enable:\n");
            int vrrEnable = 0;
            len = scanf("%d", &vrrEnable);
            if (len == 1) {
                if (meson_drm_setVrrEnabled( drmFd, req, vrrEnable, MESON_CONNECTOR_HDMIA))
                    printf("\n meson_drm_setVrr_Enabled fail:\n");
                } else {
                    printf("\n scanf fail\n");
                }
        } else if (set == 7 && len == 1) {
            printf("\n please enter the parameters in order(index zorder flag): \n");
            int zorder = 0;
            int index = 0;
            int flag = 0;
            len = scanf("%d %d %d",&index,&zorder,&flag);
            if (len == 3) {
                int ret = meson_drm_setVideoZorder(drmFd, index, zorder, flag);
                if (ret)
                    printf("\n meson_drm_setVideoZorder fail:\n");
                } else {
                    printf("\n \ scanf fail \n");
                }
        } else if (set == 8 && len == 1) {
            printf("\n please enter the parameters in order(plane_type plane_mute): \n");
            int plane_type = 0;
            int plane_mute = 0;
            len = scanf("%d %d",&plane_type,&plane_mute);
            if (len == 2) {
                int ret = meson_drm_setPlaneMute(drmFd, plane_type, plane_mute);
                if (ret)
                    printf("\n meson_drm_setPlaneMute fail:\n");
            } else {
                    printf("\n \ scanf fail \n");
            }
        } else if (set == 9) {
            printf("\n aspect ratio:\n");
            int ASPECTRATIO =-1;
            scanf("%d",&ASPECTRATIO);
            int value = meson_drm_getAspectRatioValue( drmFd, MESON_CONNECTOR_HDMIA );
            if (value == 0) {
                printf("\n current mode do not support aspect ratio change\n");
            } else {
                if (ASPECTRATIO == 1 && value == 2) {
                    if (0 == meson_drm_setAspectRatioValue(drmFd, req, ASPECTRATIO, MESON_CONNECTOR_HDMIA))
                        printf("\n aspect ratio 4:3 set success\n");
                } else if (ASPECTRATIO == 2 && value == 1) {
                    if (0 == meson_drm_setAspectRatioValue(drmFd, req, ASPECTRATIO, MESON_CONNECTOR_HDMIA))
                        printf("\n aspect ratio 16:9 set success\n");
                } else {
                    printf("\n aspect ratio invalid\n");
                }
            }
        }  else if (set == 10 && len == 1) {
            printf("\n frac rate policy\:\n");
            int fracrate = 0;
            len = scanf("%d", &fracrate);
            if (len == 1) {
                if (meson_drm_setFracRatePolicy( drmFd, req, fracrate, MESON_CONNECTOR_HDMIA))
                    printf("\n meson_drm_setFracRatePolicy fail:\n");
                } else {
                    printf("\n scanf fail\n");
                }
        }  else if (set == 11 && len == 1) {
            printf("\n please input force_output value: \n");
            int hdrforce = 0;
            len = scanf("%d", &hdrforce);
            if (len == 1) {
                if (meson_drm_setHdrForceMode(drmFd, req, hdrforce, MESON_CONNECTOR_HDMIA)) {
                    printf(" \n meson_drm_setHdrForceMode fail\n");
                } else  {
                    printf("\n meson_drm_setHdrForceMode success\n");
                }
             } else {
                 printf("\n scanf fail\n");
             }
        } else if (set == 12 && len == 1) {
            printf("\n DvMode:\n");
            int DvMode = 0;
            len = scanf("%d", &DvMode);
            if (len == 1) {
                if (meson_drm_setDvMode( drmFd, req, DvMode, MESON_CONNECTOR_HDMIA))
                    printf("\n meson_drm_setDvMode fail:\n");
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

