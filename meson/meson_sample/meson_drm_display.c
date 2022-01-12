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
#define XDG_RUNTIME_DIR 	"/run"
#endif
static int s_drm_fd = -1;
struct mesonConnector* s_conn_HDMI = NULL;

static int meson_drm_setprop(int obj_id, char* prop_name, int prop_value );
static int meson_drm_setprop(int obj_id, char* prop_name, int prop_value )
{
	int ret = -1;
	printf(" meson_drm_setprop: obj_id %d, prop_name: %s, prop_value:%d\n",obj_id, prop_name,prop_value);

	if (prop_name) {
		char cmdBuf[512] = {'\0'};
		snprintf(cmdBuf, sizeof(cmdBuf)-1, "export XDG_RUNTIME_DIR=%s;westeros-gl-console set property -s %d:%s:%d | grep \"Response\"",
				XDG_RUNTIME_DIR, obj_id, prop_name, prop_value);
		printf("Executing '%s'\n", cmdBuf);
		FILE* fp = popen(cmdBuf, "r");
		if (NULL != fp) {
			char output[64] = {'\0'};
			while (fgets(output, sizeof(output)-1, fp)) {
				if (strlen(output) && strstr(output, "[0:")) {
					ret = 0;
				}
			}
			pclose(fp);
		} else {
			printf("meson_drm_setprop: popen failed\n");
		}
	}
	return ret;
}

bool meson_drm_init()
{
	bool ret = false;
	const char *card;
	int drm_fd = -1;
	card= getenv("WESTEROS_DRM_CARD");
	if ( !card ) {
		card= DEFAULT_CARD;
	}
	s_drm_fd = open(card, O_RDONLY|O_CLOEXEC);
	if ( s_drm_fd < 0 ) {
		printf("\n drm card:%s open fail\n",card);
		ret = false;
		goto exit;
	}
	s_conn_HDMI = mesonConnectorCreate(s_drm_fd, DRM_MODE_CONNECTOR_HDMIA);
	if ( !s_conn_HDMI ) {
		printf("\n	create HDMI connector fail\n");
		ret = false;
		goto exit;
	}
	ret = true;
exit:
	return ret;
}

void meson_drm_deinit()
{
	if (s_drm_fd >= 0 )
		close(s_drm_fd);
	if (s_conn_HDMI)
		mesonConnectorDestroy(s_drm_fd,s_conn_HDMI);

}

int meson_drm_setHDMIEnable(bool enable)
{
	int ret = -1;
	int HDMI_conn_id = mesonConnectorGetId(s_conn_HDMI);
	int avmute = 1;
	if (enable)
		avmute = 0;
	else
		avmute = 1;
	if ( meson_drm_setprop(HDMI_conn_id, MESON_DRM_HDMITX_PROP_AVMUTE, avmute ) == 0 ) {
		ret = 0;
	}
	return ret;
}
bool meson_drm_getHDMIEnable()
{
	bool ret = false;
	if ( s_drm_fd >= 0 ) {
		struct mesonProperty* meson_prop = NULL;
		int HDMI_conn_id = mesonConnectorGetId(s_conn_HDMI);
		meson_prop = mesonPropertyCreate(s_drm_fd, HDMI_conn_id, DRM_MODE_OBJECT_CONNECTOR, MESON_DRM_HDMITX_PROP_AVMUTE);
		uint64_t value = mesonPropertyGetValue(meson_prop);
		if (value == 0)
			ret = true;
		else
			ret = false;
		mesonPropertyDestroy(meson_prop);
	} else {
		printf("\n drm open fail\n");
	}
	return ret;
}
int meson_drm_setVideoFormat(char* videoFormat)
{
	int ret = -1;;
	printf(" meson_drm_setVideoFormat %s\n",videoFormat);
	if (!videoFormat) {
		ret = -1;
	} else {
		char cmdBuf[512] = {'\0'};
		snprintf(cmdBuf, sizeof(cmdBuf)-1, "export XDG_RUNTIME_DIR=%s;westeros-gl-console set mode %s | grep \"Response\"",
				XDG_RUNTIME_DIR, videoFormat);
		printf("Executing '%s'\n", cmdBuf);
		/* FIXME: popen in use */
		FILE* fp = popen(cmdBuf, "r");
		if (NULL != fp) {
			char output[64] = {'\0'};
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
	}
	return ret;
}
char* meson_drm_getVideoFormat()
{
	char cmdBuf[512] = {'\0'};
	char output[64] = {'\0'};
	char* mode = NULL;
	char* mode_name = NULL;
	snprintf(cmdBuf, sizeof(cmdBuf)-1, "export XDG_RUNTIME_DIR=%s;westeros-gl-console get mode | grep \"Response\"",
			XDG_RUNTIME_DIR);
	printf("Executing '%s'\n", cmdBuf);
	/* FIXME: popen in use */
	FILE* fp = popen(cmdBuf, "r");
	if (NULL != fp) {
		while (fgets(output, sizeof(output)-1, fp)) {
			if (strlen(output) && strstr(output, "[0:")) {
				mode = strstr(output, "[0:");
				printf(" get response fail\n");
				int len = strlen(mode);
				mode_name = (char*)calloc(len, sizeof(char));
				memcpy(mode_name, mode, len);
				printf("\n len:%d  mode_name:%s\n", len, mode_name);
			}
		}
		pclose(fp);
	} else {
		printf(" popen failed\n");
	}

	return mode_name;
}
int meson_drm_setHDMItxEOTF(ENUM_DRM_HDMITX_PROP_EOTF value)
{
	int ret = -1;
	int HDMI_crtc_id = mesonConnectorGetCRTCId(s_conn_HDMI);
	if ( meson_drm_setprop(HDMI_crtc_id, MESON_DRM_HDMITX_PROP_EOTF, value ) == 0 ) {
		ret = 0;
	}
	return ret;
}
ENUM_DRM_HDMITX_PROP_EOTF meson_drm_getHDMItxEOTF()
{
	ENUM_DRM_HDMITX_PROP_EOTF ret = HDMI_EOTF_MAX;
	if ( s_drm_fd >= 0 ) {
		int HDMI_crtc_id = mesonConnectorGetCRTCId(s_conn_HDMI);
		struct mesonProperty* meson_prop = NULL;
		meson_prop = mesonPropertyCreate(s_drm_fd, HDMI_crtc_id, DRM_MODE_OBJECT_CRTC, MESON_DRM_HDMITX_PROP_EOTF);
		uint64_t value = 0;
		value = mesonPropertyGetValue(meson_prop);
		if (value == 0)
			ret = HDMI_EOTF_TRADITIONAL_GAMMA_SDR;
		else if (value == 2)
			ret = HDMI_EOTF_SMPTE_ST2084;
		else if (value == 18)
			ret = HDMI_EOTF_MESON_DOLBYVISION;
		else if (value == 19)
			ret = HDMI_EOTF_MESON_DOLBYVISION_L;
		else
			ret = HDMI_EOTF_MAX;
		mesonPropertyDestroy(meson_prop);
	} else {
		printf("\n drm open fail\n");
	}
	return ret;

}
int meson_drm_getRxSurportedModes(drmModeModeInfo **modes)
{
	//get from Rx EDID
	return 0;
}

int meson_drm_getRxPreferreddMode(drmModeModeInfo *mode)
{
	//get from EDID
	return 0;
}
int meson_drm_getRxSurportedEOTF(ENUM_DRM_HDMITX_PROP_EOTF* EOTFs)
{
	//get from EDID
	return 0;
}
ENUM_MESON_DRM_CONNECTION meson_drm_getConnection()
{
	ENUM_MESON_DRM_CONNECTION ret = MESON_DRM_UNKNOWNCONNECTION;
	if ( s_drm_fd >= 0 ) {
		if (mesonConnectorUpdate(s_drm_fd, s_conn_HDMI) == 0) {
			int ConnectState = -1;
			ConnectState = mesonConnectorGetConnectState(s_conn_HDMI);
			if (ConnectState == 1) {
				ret = MESON_DRM_CONNECTED;
			} else if (ConnectState == 2) {
				ret = MESON_DRM_DISCONNECTED;
			} else {
				ret = MESON_DRM_UNKNOWNCONNECTION;
			}
		}
	} else {
		printf("\n drm open fail\n");
	}
	return ret;
}
int meson_drm_setContentProtection(bool enable)
{
	int ret = -1;
	if ( s_drm_fd >= 0 ) {
		int HDMI_conn_id = mesonConnectorGetId(s_conn_HDMI);
		if ( meson_drm_setprop(HDMI_conn_id, DRM_CONNECTOR_PROP_CONTENT_PROTECTION, (int)enable ) == 0 ) {
			ret = 0;
		}
	} else {
		printf("\n drm open fail\n");
	}
	return ret;
}
int meson_drm_setHDCPVersion(int version)
{
	int ret = -1;
	if ( s_drm_fd >= 0 ) {
		int HDMI_conn_id = mesonConnectorGetId(s_conn_HDMI);
		if ( meson_drm_setprop(HDMI_conn_id, DRM_CONNECTOR_PROP_CONTENT_TYPE, (int)version ) == 0 ) {
			ret = 0;
		}
	} else {
		printf("\n drm open fail\n");
	}
	return ret;
}
uint64_t meson_drm_getContentProtection()
{
	uint64_t ret = 0;;
	if ( s_drm_fd >= 0 ) {
		int HDMI_conn_id = mesonConnectorGetId(s_conn_HDMI);
		struct mesonProperty* meson_prop = NULL;
		meson_prop = mesonPropertyCreate(s_drm_fd, HDMI_conn_id, DRM_MODE_OBJECT_CONNECTOR, DRM_CONNECTOR_PROP_CONTENT_PROTECTION);
		ret = mesonPropertyGetValue(meson_prop);
		mesonPropertyDestroy(meson_prop);
	} else {
		printf("\n drm open fail\n");
	}
	return ret;

}
uint64_t meson_drm_getHDCPVersion()
{
	uint64_t ret = 0;
	if ( s_drm_fd >= 0 ) {
		int HDMI_conn_id = mesonConnectorGetId(s_conn_HDMI);
		struct mesonProperty* meson_prop = NULL;
		meson_prop = mesonPropertyCreate(s_drm_fd, HDMI_conn_id, DRM_MODE_OBJECT_CONNECTOR, DRM_CONNECTOR_PROP_CONTENT_TYPE);
		ret = mesonPropertyGetValue(meson_prop);
		mesonPropertyDestroy(meson_prop);
	} else {
		printf("\n drm open fail\n");
	}
	return ret;

}

int meson_drm_getRxSupportedHDCPVersion(int* count, int** values)
{
	int ret = -1;
	if (s_drm_fd >= 0 ) {
		int HDMI_conn_id = mesonConnectorGetId(s_conn_HDMI);
		struct mesonProperty* meson_prop = NULL;
		meson_prop = mesonPropertyCreate(s_drm_fd, HDMI_conn_id, DRM_MODE_OBJECT_CONNECTOR, DRM_CONNECTOR_PROP_CONTENT_TYPE);
		int len = 0;
		struct drm_mode_property_enum* types = NULL;
		mesonPropertyGetEnumTypes(meson_prop, &len, &types);
		int i = 0;
		int* values_temp = NULL;
		*count = len;
		values_temp = (int*)calloc(len, sizeof(int));
		if (values_temp) {
			for ( i=0; i<len; i++ ) {
				values_temp[i] = (int)types[i].value;
			}
			*values = values_temp;
		}
		mesonPropertyDestroy(meson_prop);
		ret = 0;
	} else {
		printf("\n drm open fail\n");
		ret = -1;
	}
	return ret;
}

