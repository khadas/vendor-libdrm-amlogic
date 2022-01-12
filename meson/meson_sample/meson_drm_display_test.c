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

int main(void )
{
	printf("\n test meson_drm_display_test start\n");
	if (!meson_drm_init()) {
		printf("\n meson_drm_init int fail\n");
		goto exit;
	}
	int select_s_g = 0;
	printf("\n 1->get 0->set\n");
	scanf("%d",&select_s_g);
	if (select_s_g == 1) {
		printf("get value:1->HDMI enable 2->video format 3->EOTF value 4->HDMI connected 5->SupportedHDCPVersion 6->HDCP version 7->Content Protection \n");
		int get = 0;
		scanf("%d", &get);
		if (get == 1) {
			printf("\n get HDMI enable:%d\n", meson_drm_getHDMIEnable());

		} else if (get == 2) {
			char* video_format = meson_drm_getVideoFormat();
			printf("\n get HDMI :%s\n", meson_drm_getVideoFormat());
			free(video_format);

		} else if (get == 3) {
			printf("\n EOTF:%d\n",meson_drm_getHDMItxEOTF());

		} else if (get == 4) {
			printf("\n HDMI connected:%d\n",meson_drm_getConnection());
		} else if (get ==5 ) {
			int len = 0;
			int i = 0;
			int* types = NULL;
			if (meson_drm_getRxSupportedHDCPVersion(&len, &types) == 0) {
				for ( i=0; i<len; i++ ) {
					printf("\n %d\n", types[i]);
				}
			}
		} else if (get == 6) {
			uint64_t value = meson_drm_getHDCPVersion();
			printf("\n HDMI version:%llu\n",value);

		} else if (get == 7) {
			uint64_t value = meson_drm_getContentProtection();
			printf("\n Content Protection:%llu\n", value);
		} else {
			printf("\n invalid input\n");
		}

	} else {
		printf("get value:1->HDMI enable 2->video format 3->EOTF value 4->Content Protection 5->HDCP Version \n");
		int set = 0;
		scanf("%d", &set);
		if (set == 1) {
			printf("\n 0->disable HDMI AV. 1->enable HDMI AV\n");
			int enable = 1;
			scanf("%d",&enable);
			if (meson_drm_setHDMIEnable( (bool)enable ) == 0)
				printf("\n set HDMI enable success\n");

		} else if (set == 2) {
			printf("\n set video format\n");
			int enable = 1;
			getchar();
			char video_format[30] = {0};
			gets(video_format);
			if (meson_drm_setVideoFormat( video_format ) == 0)
				printf("\n set Video Format success\n");

		} else if (set == 3) {
			printf("\n HDMI_EOTF_TRADITIONAL_GAMMA_SDR = 0 \n"
					 " HDMI_EOTF_SMPTE_ST20842 = 2 \n"
					 " HDMI_EOTF_NODV_HDR_SDR = 21 \n"
					 " HDMI_EOTF_NODV_SDR_HDR = 22 \n"
				);
			int hdr = 1;
			scanf("%d",&hdr);
			if (meson_drm_setHDMItxEOTF( (ENUM_DRM_HDMITX_PROP_EOTF)hdr ) == 0)
				printf("\n set HDMI eotf success\n");

		} else if (set == 4) {
			printf("\n 0->disable content protection. 1->enable content protection\n");
			int enable = 1;
			scanf("%d",&enable);
			if (meson_drm_setContentProtection( (bool)enable ) == 0)
				printf("\n set HDMI enable success\n");

		} else if (set == 5) {
			printf("\n version:\n");
			int version = 1;
			scanf("%d",&version);
			if (meson_drm_setHDCPVersion( version ) == 0)
				printf("\n set HDCP Version success\n");
		}
	}
	meson_drm_deinit();
	getchar();
exit:
	return 0;
}

