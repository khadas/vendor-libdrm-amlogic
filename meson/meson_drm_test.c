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

#define DEFAULT_CARD "/dev/dri/card0"
static int print_Mode(drmModeModeInfo *mode);
static void meson_drm_connector_test(int drm_fd, struct mesonConnector* meson_conn);
static void meson_drm_prop_test(int drmFd, struct mesonProperty* meson_prop);
static int meson_drm_setprop(uint32_t obj_id, char* prop_name, int prop_value );

static int print_Mode(drmModeModeInfo *mode)
{
	if (mode) {
		printf("\n\nMode: %s\n", mode->name);
		printf("\tclock 	  : %i\n", mode->clock);
		printf("\thdisplay	  : %i\n", mode->hdisplay);
		printf("\thsync_start : %i\n", mode->hsync_start);
		printf("\thsync_end   : %i\n", mode->hsync_end);
		printf("\thtotal	  : %i\n", mode->htotal);
		printf("\thskew 	  : %i\n", mode->hskew);
		printf("\tvdisplay	  : %i\n", mode->vdisplay);
		printf("\tvsync_start : %i\n", mode->vsync_start);
		printf("\tvsync_end   : %i\n", mode->vsync_end);
		printf("\tvtotal	  : %i\n", mode->vtotal);
		printf("\tvscan 	  : %i\n", mode->vscan);
		printf("\tvrefresh	  : %i\n", mode->vrefresh);
		printf("\ttype 	  : %i\n", mode->type);
		if ( mode->type & DRM_MODE_TYPE_PREFERRED )
			printf(" prefered!! ");
	} else {
		printf("\n print_Mode fail\n");
	}
	return 0;
}
static void meson_drm_connector_test(int drm_fd, struct mesonConnector* meson_conn )
{
	int conn_id = -1;
	drmModeModeInfo* modes = NULL;
	int mode_count = -1;
	int i;
	int edid_data_len = -1;
	char* edid_data = NULL;
	int ret = -255;
	int conn_connect = -1;
	int crtc_id = -1;
	conn_id = mesonConnectorGetId(meson_conn);
	printf("\n conn_id:%d\n",conn_id);
	ret = mesonConnectorGetModes(meson_conn, drm_fd, &modes, &mode_count);
	if (ret == 0) {
		printf("\n mode_count:%d modes:%p\n",mode_count,modes);
		for (i=0; i<mode_count; i++)
			print_Mode(&(modes[i]));
	} else {
		printf("\n mesonConnectorGetModes return fail:%d\n", ret);
	}
	printf("\n EDID info:\n");
	ret = -255;

	ret = mesonConnectorGetEdidBlob(meson_conn, &edid_data_len, &edid_data);
	if (ret == 0) {
		 for (i = 0; i < edid_data_len; i++) {
			if (i % 16 == 0)
				printf("\n\t\t\t");
			if (edid_data)
				printf("%.2hhx", edid_data[i]);
		}
		printf("\nedid_data:%p\n",edid_data);
	}
	conn_connect = mesonConnectorGetConnectState(meson_conn);
	printf("\n connected:%d\n",conn_connect);
	crtc_id = mesonConnectorGetCRTCId(meson_conn);
	printf("\n CRTC id:%d\n",crtc_id);
}
static void meson_drm_prop_test(int drmFd, struct mesonProperty* meson_prop)
{
	int prop_id = -1;
	uint64_t value = 0;
	int ret = -1;
	int i;

	prop_id = mesonPropertyGetId(meson_prop);
	printf("\n prop id:%d\n", prop_id);
	value = mesonPropertyGetValue(meson_prop);
	printf("\n value:%llu\n", value);
	if ( (mesonPropertyGetType(meson_prop) & DRM_MODE_PROP_SIGNED_RANGE) || (mesonPropertyGetType(meson_prop) & DRM_MODE_PROP_RANGE) ) {
		uint64_t min = 0;
		uint64_t max = 0;
		printf("\n prop type is DRM_MODE_PROP_RANGE or DRM_MODE_PROP_SIGNED_RANGE\n");
		ret = mesonPropertyGetRange(meson_prop, &min, &max);
		if (ret == 0) {
			printf("\n range:min(%llu),max(%llu)\n", min, max);
		}
	}
	if (mesonPropertyGetType(meson_prop) & DRM_MODE_PROP_ENUM) {
		int len =-1;
		printf("\n prop type is DRM_MODE_PROP_ENUM\n");
		struct drm_mode_property_enum* enums = NULL;
		ret = mesonPropertyGetEnumTypes(meson_prop, &len, &enums);
		if (ret == 0) {
			for ( i=0; i<len; i++ )
				printf("\n enum name(%s) value(%llu)\n", enums[i].name, enums[i].value);
		}
	}
	if (mesonPropertyGetType(meson_prop) & DRM_MODE_PROP_BLOB) {
		int blob_count = -1;
		printf("\n prop type is DRM_MODE_PROP_BLOB\n");
		char* blob_data = NULL;
		ret = mesonPropertyGetBlobData(meson_prop, drmFd, &blob_count, &blob_data);
		if (ret == 0) {
			for ( i=0; i<blob_count; i++) {
				if (i % 16 == 0)
					printf("\n\t\t");
				printf("%.2hhx", blob_data[i]);
			}
		}
		printf("\n");

	}
}
#ifndef XDG_RUNTIME_DIR
#define XDG_RUNTIME_DIR     "/run"
#endif

static int meson_drm_setprop(uint32_t obj_id, char* prop_name, int prop_value )
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

int main(void )
{
	printf("\n test moson_drm_test start\n");
	const char *card;
	int drm_fd = -1;
	uint32_t object_type = 0;
	card= getenv("WESTEROS_DRM_CARD");
	if ( !card ) {
		card= DEFAULT_CARD;
	}
	printf("\n drm card:%s\n",card);
	drm_fd = open(card, O_RDONLY|O_CLOEXEC);
	if ( drm_fd < 0 ) {
		printf("\n drm card:%s open fail\n",card);
		goto exit;
	}
	struct mesonConnector* meson_conn = NULL;
	meson_conn = mesonConnectorCreate(drm_fd, DRM_MODE_CONNECTOR_HDMIA);
	meson_drm_connector_test(drm_fd, meson_conn);

	struct mesonConnector* meson_conn_CVBS = NULL;
	meson_conn_CVBS = mesonConnectorCreate(drm_fd, DRM_MODE_CONNECTOR_TV);
	meson_drm_connector_test(drm_fd, meson_conn_CVBS);
	mesonConnectorDestroy(drm_fd,meson_conn_CVBS);
	printf("\n connector change wait 10s\n");
	sleep(10);
	mesonConnectorUpdate(drm_fd, meson_conn);
	meson_drm_connector_test(drm_fd, meson_conn);

	uint32_t obj_id =mesonConnectorGetId(meson_conn);
	uint32_t obj_type = DRM_MODE_OBJECT_CONNECTOR;
	printf("\n\n\n start to test prop\n");

	printf("\n please select set/get prop, set->0, get->1\n");
	int select_s_g = 0;
	scanf("%d",&select_s_g);
	if (select_s_g == 1) {
		printf("\n please input object type:0->DRM_MODE_OBJECT_CONNECTOR, 1->DRM_MODE_OBJECT_CRTC\n");
		scanf("%d",&object_type);
		if (object_type == 0)
			obj_type = DRM_MODE_OBJECT_CONNECTOR;
		else if (object_type == 1)
			obj_type = DRM_MODE_OBJECT_CRTC;
		else
			obj_type = DRM_MODE_OBJECT_CONNECTOR;
		printf("\n please input object id: \n");
		uint32_t object = 0;
		scanf("%d",&object);
		getchar();

		printf("\n please input prop name: \n");
		char name[30] = {0};
		gets(name);
		struct mesonProperty* meson_prop_test = NULL;
		printf("\n drmfd:%d, drmObject:%u, drmObjectType:%u,name:%s\n",drm_fd, object, obj_type, name );
		meson_prop_test = mesonPropertyCreate(drm_fd, object, obj_type, name);
		meson_drm_prop_test(drm_fd, meson_prop_test);
		mesonPropertyDestroy(meson_prop_test);
	} else {
		printf("\n please input object type:0->DRM_MODE_OBJECT_CONNECTOR, 1->DRM_MODE_OBJECT_CRTC\n");
		scanf("%d",&object_type);
		if (object_type == 0)
			obj_type = DRM_MODE_OBJECT_CONNECTOR;
		else if (object_type == 1)
			obj_type = DRM_MODE_OBJECT_CRTC;
		else
			obj_type = DRM_MODE_OBJECT_CONNECTOR;
		printf("\n please input object id: \n");
		uint32_t object = 0;
		scanf("%d",&object);
		getchar();

		printf("\n please input prop name: \n");
		char name[30] = {0};
		gets(name);

		printf("\n please input prop value: \n");
		int value = 0;
		scanf("%d",&value);
		struct mesonProperty* meson_prop_test_set = NULL;
		printf("\n set prop drmfd:%d, drmObject:%u, drmObjectType:%u,name:%s value:%d\n",drm_fd, object, obj_type, name, value );
		meson_prop_test_set = mesonPropertyCreate(drm_fd, object, obj_type, name);
		printf("\n print before set !\n");
		meson_drm_prop_test(drm_fd, meson_prop_test_set);
		int ret_t = -255;
		ret_t = meson_drm_setprop(obj_id, name, value);
		ret_t = mesonPropertyUpdateValue(drm_fd, meson_prop_test_set);
		if (ret_t == 0)
			printf("\n mesonPropertyUpdateValue %s success! \n",name);

		printf("\n print after set !\n");
		meson_drm_prop_test(drm_fd, meson_prop_test_set);
		mesonPropertyDestroy(meson_prop_test_set);
	}

	mesonConnectorDestroy(drm_fd, meson_conn);
	close(drm_fd);
	exit:
	return 0;
}

