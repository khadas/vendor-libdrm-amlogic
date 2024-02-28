/*
 * Copyright (c) 2021 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#include <errno.h>
#include <string.h>
#include "libdrm_meson_connector.h"
#include <linux/version.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <linux/version.h>
#include "meson_drm_log.h"

#define MESON_DRM_MODE_GETCONNECTOR_HDMI     "/sys/class/drm/card0-HDMI-A-1/status"
#define MESON_DRM_MODE_GETCONNECTOR_VBYONE    "/sys/class/drm/card0-VBYONE-A/status"

struct mesonConnector {
int type;
int id;
int count_modes;
drmModeModeInfo* modes;
int edid_data_Len;
char* edid_data;
int connection;
int crtc_id;
int encoder_id;
uint32_t mm_width;
uint32_t mm_height;
};
static int meson_amsysfs_set_sysfs_strs(const char *path, const char *val)
{
	int fd;
	int bytes;
	fd = open(path, O_CREAT | O_WRONLY | O_TRUNC, 0644);
	if (fd >= 0) {
		bytes = write(fd, val, strlen(val));
		close(fd);
		if (bytes != strlen(val))
			return -1;
		else
			return 0;
	}
	return -1;
}

struct mesonPrimaryPlane {
int fb_width;
int fb_height;
};

struct mesonPrimaryPlane* mesonPrimaryPlaneCreate(int drmFd) {
	drmModePlane *plane= 0;
	drmModeObjectProperties *props= 0;
	drmModePropertyRes *prop= 0;
	drmModeFB *fb = 0;
	drmModePlaneRes *planeRes= 0;
	struct mesonPrimaryPlane* ret = NULL;
	planeRes = drmModeGetPlaneResources(drmFd);
	ret = (struct mesonPrimaryPlane*)calloc( 1,sizeof(struct mesonPrimaryPlane) );
	if ( !ret ) {
		ERROR("%s %d mesonPrimaryPlane create fail",__FUNCTION__,__LINE__);
		goto out;
	}
	if ( !planeRes ) {
		ERROR("%s %d failed to get plane resources from drmFd (%d)", __FUNCTION__,__LINE__,drmFd);
		free(ret);
		ret = NULL;
		goto out;
	}
	for ( uint32_t n = 0; n < planeRes->count_planes; ++n ) {
		plane = drmModeGetPlane(drmFd, planeRes->planes[n]);
		if ( !plane ) {
			ERROR("%s %d drmModeGetPlane failed", __FUNCTION__,__LINE__);
			free(ret);
			ret = NULL;
			goto out;
		}
		props = drmModeObjectGetProperties( drmFd, planeRes->planes[n], DRM_MODE_OBJECT_PLANE );
		if ( !props ) {
			ERROR("%s %d drmModeObjectGetProperties failed",__FUNCTION__,__LINE__);
			free(ret);
			ret = NULL;
			goto out;
		}
		for ( uint32_t j = 0; j < props->count_props; ++j ) {
			prop = drmModeGetProperty( drmFd, props->props[j] );
			if ( !prop ) {
				free(ret);
				ret = NULL;
				goto out;
			}
			if (!strcmp(prop->name, "type")) {
				if (props->prop_values[j] == DRM_PLANE_TYPE_PRIMARY) {
					/* get fb */
					fb = drmModeGetFB(drmFd, plane->fb_id);
					if (fb) {
					/* Get the width and height */
						ret->fb_width = fb->width;
						ret->fb_height = fb->height;
						drmModeFreeFB(fb);
					}
				}
			}
			drmModeFreeProperty(prop);
		}
	}
out:
	if (props) {
		drmModeFreeObjectProperties( props );
	}
	if (plane) {
		drmModeFreePlane( plane );
		plane = 0;
	}
	drmModeFreePlaneResources( planeRes );
	return ret;
}

struct mesonConnector *mesonConnectorCreate(int drmFd, int type)
{
	drmModeRes *res= NULL;
	drmModeConnector *conn= NULL;
	struct mesonConnector* ret = NULL;
	drmModePropertyRes *prop_ptr = NULL;
	drmModePropertyBlobRes* edid_blob = NULL;
	char *edid_data_temp = NULL;
	drmModeModeInfo* modes_info = NULL;
	drmModeEncoder* cur_encoder = NULL;
	int i = 0;
	int j = 0;
	int blob_id = 0;
	res= drmModeGetResources( drmFd );
	ret = (struct mesonConnector*)calloc( 1,sizeof(struct mesonConnector) );
	if ( !ret ) {
		ERROR("%s %d mesonConnector create fail",__FUNCTION__,__LINE__);
		goto exit;
	}
	if ( !res )
	{
		ERROR("%s %d failed to get resources from drmFd (%d)", __FUNCTION__,__LINE__,drmFd);
		free(ret);
		ret = NULL;
		goto exit;
	}
	for ( i= 0; i < res->count_connectors; ++i ) {
		if ( type == DRM_MODE_CONNECTOR_HDMIA ) {
			if (0 != meson_amsysfs_set_sysfs_strs(MESON_DRM_MODE_GETCONNECTOR_HDMI, "detect"))
				ERROR("\n %s %d detect set fail for HDMIA\n", __FUNCTION__,__LINE__);
	   } else if ( type == DRM_MODE_CONNECTOR_LVDS ) {
			if (0 != meson_amsysfs_set_sysfs_strs(MESON_DRM_MODE_GETCONNECTOR_VBYONE, "detect"))
				ERROR("\n %s %d detect set fail for LVDS\n", __FUNCTION__,__LINE__);
	   }
	   conn= drmModeGetConnector( drmFd, res->connectors[i] );
	   if ( conn ) {
		  if ( conn->connector_type == type ) {    //是否需要判断当前connector是否连接
			 break;
		  }
		  drmModeFreeConnector(conn);
		  conn= NULL;
	   }
	}
	if ( !conn ) {
		free(ret);
		ret = NULL;
		goto exit;
	}
	if ( conn ) {
		ret->id = conn->connector_id;
		ret->type = conn->connector_type;
		ret->encoder_id = conn->encoder_id;
		ret->mm_width = conn->mmWidth;
		ret->mm_height = conn->mmHeight;
		cur_encoder = drmModeGetEncoder(drmFd, ret->encoder_id);
		if (cur_encoder)
			ret->crtc_id = cur_encoder->crtc_id;
		else {
			DEBUG("%s %d cur encoder not exit, get crtcs[0]:%d ",__FUNCTION__,__LINE__,res->crtcs[0]);
			ret->crtc_id = res->crtcs[0];
		}
		drmModeFreeEncoder(cur_encoder);
		ret->count_modes = conn->count_modes;
		if (ret->count_modes > 0) {
			modes_info =  (drmModeModeInfo*)malloc( ret->count_modes * sizeof(drmModeModeInfo) );
			if (!modes_info) {
				ERROR("%s %d calloc fail",__FUNCTION__,__LINE__);
				goto exit;
			}
			memcpy(modes_info, conn->modes, ret->count_modes *sizeof(drmModeModeInfo));
		}
		ret->modes = modes_info;
		ret->connection = conn->connection;
		if (conn->props) {
			for (j = 0; j < conn->count_props; j++ ) {
				prop_ptr = drmModeGetProperty(drmFd,conn->props[j]);
				if ( prop_ptr ) {
					if ( strcmp(prop_ptr->name,"EDID" ) == 0 ) {
						blob_id = conn->prop_values[j];
						edid_blob = drmModeGetPropertyBlob(drmFd, blob_id);
						if (edid_blob ) {
							ret->edid_data_Len = edid_blob->length;
							edid_data_temp = (char*)calloc( ret->edid_data_Len,sizeof(char) );
							memcpy(edid_data_temp, edid_blob->data, ret->edid_data_Len);
							ret->edid_data = edid_data_temp;
						}
						drmModeFreePropertyBlob(edid_blob);
					}
				}
				drmModeFreeProperty(prop_ptr);
			}
		}
	}
exit:
	drmModeFreeResources(res);
	drmModeFreeConnector(conn);
	return ret;
}
int mesonConnectorUpdate(int drmFd, struct mesonConnector *connector)
{
	int ret = -1;
	drmModeConnector *conn= NULL;
	drmModePropertyBlobRes* edid_blob = NULL;
	drmModePropertyRes *prop_ptr = NULL;
	char* edid_data_temp = NULL;
	drmModeModeInfo* modes_info = NULL;
	drmModeEncoder* cur_encoder = NULL;
	int blob_id = 0;
	int j = 0;
	if ( !connector ) {
		ERROR("%s %d invalid parameters ",__FUNCTION__,__LINE__);
		goto exit;
	}
	conn= drmModeGetConnector( drmFd, connector->id );
	if ( !conn ) {
		ERROR("%s %d mesonConnectorCreate: unable to get connector for drmfd (%d) conn_id(%d)\n",__FUNCTION__,__LINE__, drmFd,connector->id);
	goto exit;
	}
	if ( conn ) {
		connector->id = conn->connector_id;
		connector->type = conn->connector_type;
		connector->encoder_id = conn->encoder_id;
		cur_encoder = drmModeGetEncoder(drmFd, connector->encoder_id);
		if (cur_encoder)
			connector->crtc_id = cur_encoder->crtc_id;
		drmModeFreeEncoder(cur_encoder);
		connector->count_modes = conn->count_modes;
		if (connector->modes) {
			free(connector->modes);
			connector->modes = NULL;
		}
		if ( connector->count_modes > 0 ) {
			modes_info =  (drmModeModeInfo*)calloc( connector->count_modes, sizeof(drmModeModeInfo) );
			if (modes_info)
				memcpy(modes_info, conn->modes, connector->count_modes *sizeof(drmModeModeInfo));
		}
		connector->modes = modes_info;
		connector->connection = conn->connection;
		if (connector->edid_data) {
			free(connector->edid_data);
			connector->edid_data = NULL;
		}
		if (conn->props) {
			for (j = 0; j < conn->count_props; j++ ) {
				prop_ptr = drmModeGetProperty(drmFd,conn->props[j]);
				if ( prop_ptr ) {
					if ( strcmp(prop_ptr->name,"EDID") == 0 ) {
						blob_id = conn->prop_values[j];
						edid_blob = drmModeGetPropertyBlob(drmFd, blob_id);
						if (edid_blob ) {
							connector->edid_data_Len = edid_blob->length;
							edid_data_temp = (char*)calloc( connector->edid_data_Len,sizeof(char) );
							memcpy(edid_data_temp, edid_blob->data, connector->edid_data_Len);
							connector->edid_data = edid_data_temp;
						}
						drmModeFreePropertyBlob(edid_blob);
					}
				}
				drmModeFreeProperty(prop_ptr);
			}
		}
		ret = 0;
	}
exit:
	drmModeFreeConnector(conn);
	return ret;
}
struct mesonConnector *mesonConnectorDestroy(int drmFd, struct mesonConnector *connector)
{
	if ( connector ) {
		if (connector->edid_data)
			free(connector->edid_data);
		if (connector->modes)
			free(connector->modes);
		connector->edid_data = NULL;
		connector->modes = NULL;
		free(connector);
		connector = NULL;
	}
	return connector;
}

void mesonPrimaryPlaneDestroy(int drmFd, struct mesonPrimaryPlane *primaryplane)
{
	if ( primaryplane ) {
		free(primaryplane);
		primaryplane = NULL;
	}
}

int mesonConnectorGetId(struct mesonConnector* connector)
{
	if ( connector ) {
		return connector->id;
	} else {
		ERROR("%s %d invalid parameters",__FUNCTION__,__LINE__);
		return 0;
	}
}
/*HDMI capabilities - Rx supported video formats*/
int mesonConnectorGetModes(struct mesonConnector * connector, int drmFd, drmModeModeInfo **modes, int *count_modes)
{
	int ret = -1;
	if ( !connector || !count_modes ) {
		ERROR("%s %d invalid parameters ",__FUNCTION__,__LINE__);
	} else {
		*count_modes = connector->count_modes;
		*modes = connector->modes;
		ret = 0;
	}
	return ret;
}
/*Edid Information:*/
int mesonConnectorGetEdidBlob(struct mesonConnector* connector, int * data_Len, char **data)
{
	int ret = -1;
	if ( !connector || !data_Len ) {
		ERROR("%s %d invalid parameters ",__FUNCTION__,__LINE__);
	} else {
		*data_Len = connector->edid_data_Len;
		*data = connector->edid_data;
		ret = 0;
	}
	 return ret;
}
/*get connector connect status.*/
int mesonConnectorGetConnectState(struct mesonConnector* connector)
{
	int ret = -1;
	if ( !connector ) {
			ERROR("%s %d invalid parameters ",__FUNCTION__,__LINE__);
	} else {
		ret = connector->connection;
	}
	 return ret;
}
int mesonConnectorGetCRTCId(struct mesonConnector* connector)
{
	int ret = -1;
	if ( !connector ) {
		ERROR("%s %d invalid parameters ",__FUNCTION__,__LINE__);
	} else {
		ret = connector->crtc_id;
	}
	 return ret;
}
drmModeModeInfo* mesonConnectorGetCurMode(int drmFd, struct mesonConnector* connector)
{
	drmModeModeInfo* mode = NULL;
	drmModeCrtcPtr crtc;
	mode = (drmModeModeInfo*)calloc(1, sizeof(drmModeModeInfo));
	crtc = drmModeGetCrtc(drmFd, connector->crtc_id);
	if (crtc && mode) {
		memcpy(mode, &crtc->mode, sizeof(drmModeModeInfo));
	}
	drmModeFreeCrtc(crtc);
	return mode;
}

void dump_connector(struct mesonConnector* connector)
{
	int i = 0;
	int j = 0;
	drmModeModeInfo* mode = NULL;
	if (connector) {
		DEBUG("%s %d connector",__FUNCTION__,__LINE__);
		DEBUG("%s %d id: %d",__FUNCTION__,__LINE__,connector->id);
		DEBUG("%s %d type: %d",__FUNCTION__,__LINE__,connector->type);
		DEBUG("%s %d connection: %d",__FUNCTION__,__LINE__,connector->connection);
		DEBUG("%s %d count_modes: %d",__FUNCTION__,__LINE__,connector->count_modes);
		DEBUG("%s %d modes: ",__FUNCTION__,__LINE__);
		DEBUG("%s %d \tname refresh (Hz) hdisp hss hse htot vdisp "
				   "vss vse vtot)",__FUNCTION__,__LINE__);
		for (j = 0; j < connector->count_modes; j++) {
			mode = &connector->modes[j];
			INFO("%s %d : %s %d %d %d %d %d %d %d %d %d %d",__FUNCTION__,__LINE__,
			mode->name,
			mode->vrefresh,
			mode->hdisplay,
			mode->hsync_start,
			mode->hsync_end,
			mode->htotal,
			mode->vdisplay,
			mode->vsync_start,
			mode->vsync_end,
			mode->vtotal,
			mode->clock);
		}
		DEBUG("%s %d edid_data_Len: %d",__FUNCTION__,__LINE__,connector->edid_data_Len);
		DEBUG("%s %d edid",__FUNCTION__,__LINE__);
		for (i = 0; i < connector->edid_data_Len; i++) {
			if (i % 16 == 0)
				DEBUG("\n\t\t\t");
			INFO("%s %d %.2hhx",__FUNCTION__,__LINE__, connector->edid_data[i]);
		}
		DEBUG("\n");
	}
}

/*   Get fb the width and height Information */
int mesonPrimaryPlaneGetFbSize(struct mesonPrimaryPlane* planesize, int* width, int* height) {
	int ret = -1;
	if (!planesize) {
		ERROR("%s %d invalid parameters ",__FUNCTION__,__LINE__);
	} else {
		*width = planesize->fb_width;
		*height = planesize->fb_height;
		ret = 0;
		DEBUG("%s %d GetGraphicPlaneSize %d x %d",__FUNCTION__,__LINE__,*width, *height);
	}
	return ret;
}

/*   Get physical size Information */
int mesonConnectorGetPhysicalSize(struct mesonConnector * connector, int* width, int* height) {
	int ret = -1;
	if (!connector) {
		ERROR("%s %d invalid parameters ",__FUNCTION__,__LINE__);
	} else {
		*width = connector->mm_width;
		*height = connector->mm_height;
		ret = 0;
	}
	return ret;
}


