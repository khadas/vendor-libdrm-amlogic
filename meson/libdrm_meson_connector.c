/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#include <string.h>
#include "libdrm_meson_connector.h"
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
};

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
		printf("\n mesonConnectorCreate:mesonConnector create fail \n");
		goto exit;
	}
	if ( !res )
	{
		printf("\n mesonConnectorCreate: failed to get resources from drmFd (%d)", drmFd);
		ret = NULL;
		goto exit;
	}
	for ( i= 0; i < res->count_connectors; ++i ) {
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
		printf("\n mesonConnectorCreate: unable to get connector for drmfd (%d) type(%d)\n", drmFd,type);
		ret = NULL;
		goto exit;
	}
	if ( conn ) {
		ret->id = conn->connector_id;
		ret->type = conn->connector_type;
		ret->encoder_id = conn->encoder_id;
		cur_encoder = drmModeGetEncoder(drmFd, ret->encoder_id);
		if (cur_encoder)
			ret->crtc_id = cur_encoder->crtc_id;
		drmModeFreeEncoder(cur_encoder);
		ret->count_modes = conn->count_modes;
		if (ret->count_modes > 0) {
			modes_info =  (drmModeModeInfo*)calloc( ret->count_modes, sizeof(drmModeModeInfo) );
			if (!modes_info) {
				printf("\n mesonConnectorCreate:calloc fail\n");
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
	conn= drmModeGetConnector( drmFd, connector->id );
	//ret = (struct mesonConnector*)calloc( 1,sizeof(struct mesonConnector) );
	if ( !connector ) {
		printf("\n mesonConnectorUpdate:invalid parameters\n");
		goto exit;
	}
	if ( !ret ) {
		printf("\n mesonConnectorUpdate:mesonConnector create fail\n");
		goto exit;
	}
	if ( !conn ) {
		printf("\n mesonConnectorCreate: unable to get connector for drmfd (%d) conn_id(%d)\n", drmFd,connector->id);
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
int mesonConnectorGetId(struct mesonConnector* connector)
{
	if ( connector ) {
		return connector->id;
	} else {
		printf("\n mesonConnectorGetId:invalid parameters\n");
		return 0;
	}
}
/*HDMI capabilities - Rx supported video formats*/
int mesonConnectorGetModes(struct mesonConnector * connector, int drmFd, drmModeModeInfo **modes, int *count_modes)
{
	int ret = -1;
	if ( !connector || !count_modes ) {
		printf("\n mesonConnectorGetModes:invalid parameters\n");
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
		printf("\n mesonConnectorGetEdidBlob:invalid parameters\n");
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
			printf("\n mesonConnectorGetEdidBlob:invalid parameters\n");
	} else {
		ret = connector->connection;
	}
	 return ret;
}
int mesonConnectorGetCRTCId(struct mesonConnector* connector)
{
	int ret = -1;
	if ( !connector ) {
		printf("\n mesonConnectorGetEdidBlob:invalid parameters\n");
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
		printf("\n connector:\n");
		printf("id:%d\n",connector->id);
		printf("type:%d\n",connector->type);
		printf("connection:%d\n",connector->connection);
		printf("count_modes:%d\n",connector->count_modes);
		printf("modes:\n");
		printf("\tname refresh (Hz) hdisp hss hse htot vdisp "
				   "vss vse vtot)\n");
		for (j = 0; j < connector->count_modes; j++) {
			mode = &connector->modes[j];
			printf("  %s %d %d %d %d %d %d %d %d %d %d",
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
		printf("edid_data_Len:%d\n",connector->edid_data_Len);
		printf("edid\n");
		for (i = 0; i < connector->edid_data_Len; i++) {
			if (i % 16 == 0)
				printf("\n\t\t\t");
			printf("%.2hhx", connector->edid_data[i]);
		}
		printf("\n");
	}
}
