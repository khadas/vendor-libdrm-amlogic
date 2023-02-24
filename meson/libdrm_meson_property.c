 /*
 * Copyright (c) 2021 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#include <string.h>
#include "libdrm_meson_property.h"
struct mesonProperty {
	drmModePropertyPtr prop_ptr;
	uint64_t mValue;
	uint32_t mType;
	uint32_t mComponentId;
	uint32_t drmObjectType;
};
struct mesonProperty *mesonPropertyCreate(int drmFd, uint32_t drmObject, uint32_t drmObjectType, char* name)
{
	int i = 0;
	struct mesonProperty* ret = NULL;
	drmModeObjectPropertiesPtr props;
	drmModePropertyPtr prop = NULL;
	drmModePropertyPtr prop_temp = NULL;
	props = drmModeObjectGetProperties(drmFd, drmObject, drmObjectType);
	struct drm_mode_property_enum *enums_temp = NULL;
	uint64_t* values_temp = NULL;
	uint32_t* blob_ids_temp = NULL;
	if ( !props ) {
		printf("\t No properties:mesonPropertyCreate \n");
		goto exit;
	}
	prop = (drmModePropertyPtr)calloc( 1, sizeof(drmModePropertyRes));
	if ( !prop ) {
		printf("\t mesonPropertyCreate alloc fail\n");
		goto exit;
	}
	ret = (struct mesonProperty*)calloc( 1,sizeof(struct mesonProperty) );
	if ( !ret ) {
		printf("\t mesonPropertyCreate alloc mesonProperty fail\n");
		goto exit;
	}
	for (i = 0; i < props->count_props; i++) {
		prop_temp = drmModeGetProperty(drmFd,props->props[i] );
		if ( !prop_temp )
			continue;
		if ( strcmp(prop_temp->name, name) == 0 ) {
			memcpy( prop, prop_temp, sizeof(drmModePropertyRes) );
			ret->prop_ptr = prop;
			if (prop->count_enums > 0) {
				enums_temp = (struct drm_mode_property_enum*)calloc( prop->count_enums, sizeof(struct drm_mode_property_enum));
				if (enums_temp)
					memcpy( enums_temp, prop->enums, prop->count_enums*sizeof(struct drm_mode_property_enum) );
			}
			ret->prop_ptr->enums = enums_temp;

			if ( prop->count_values > 0 ) {
				values_temp = (uint64_t*)calloc( prop->count_values, sizeof(uint64_t));
				if (values_temp)
					memcpy( values_temp, prop->values, prop->count_values*sizeof(uint64_t) );
			}
			ret->prop_ptr->values = values_temp;

			if ( prop->count_blobs > 0 ) {
				blob_ids_temp = (uint32_t*)calloc( prop->count_blobs, sizeof(uint32_t));
				if (blob_ids_temp)
					memcpy(blob_ids_temp, prop->blob_ids, prop->count_blobs*sizeof(uint32_t));
			}
			ret->prop_ptr->blob_ids = blob_ids_temp;
			ret->mComponentId = drmObject;
			ret->mValue = props->prop_values[i];
			ret->mType = prop->flags;
			if ( drm_property_type_is(prop, DRM_MODE_PROP_BLOB) )
				ret->mType = DRM_MODE_PROP_BLOB;
			if ( drm_property_type_is(prop, DRM_MODE_PROP_SIGNED_RANGE) )
				ret->mType = DRM_MODE_PROP_SIGNED_RANGE;
			if ( drm_property_type_is(prop, DRM_MODE_PROP_RANGE) )
				ret->mType = DRM_MODE_PROP_RANGE;
			if ( drm_property_type_is(prop, DRM_MODE_PROP_ENUM) )
				ret->mType = DRM_MODE_PROP_ENUM;
			if ( drm_property_type_is(prop, DRM_MODE_PROP_ENUM) )
				ret->mType = DRM_MODE_PROP_ENUM;
			ret->drmObjectType = drmObjectType;
			break;
		}
		drmModeFreeProperty(prop_temp);
		prop_temp = NULL;
	}

	if ( !prop_temp ) {
		free(ret);
		ret = NULL;
	}
exit:
	if ( ret == NULL && prop != NULL ) {
		free(prop);
	}
	drmModeFreeProperty(prop_temp);
	drmModeFreeObjectProperties(props);
	return ret;
}
/*update cached value*/
int mesonPropertyUpdateValue(int drmFd, struct mesonProperty *mesonProp)
{
	int ret = -1;
	int i = 0;
	drmModeObjectPropertiesPtr props = NULL;
	drmModePropertyPtr prop_temp = NULL;
	struct drm_mode_property_enum *enums_temp = NULL;
	uint64_t* values_temp = NULL;
	uint32_t* blob_ids_temp = NULL;
	if ( !mesonProp ) {
		printf("\t mesonPropertyUpdateValue invalid parameter\n");
		ret = -1;
		goto exit;
	}
	props = drmModeObjectGetProperties(drmFd, mesonProp->mComponentId, mesonProp->drmObjectType);
	if ( !props ) {
		printf("\t No properties:mesonPropertyUpdateValue \n");
		ret = -1;
		goto exit;
	}
	if ( mesonProp ) {
		for (i = 0; i < props->count_props; i++) {
			prop_temp = drmModeGetProperty(drmFd,props->props[i] );
			if ( !prop_temp )
				continue;
			if ( prop_temp->prop_id == mesonProp->prop_ptr->prop_id ) {
				if ( mesonProp->prop_ptr->values ) {
					free(mesonProp->prop_ptr->values);
					mesonProp->prop_ptr->values = NULL;
				}
				if (mesonProp->prop_ptr->enums ) {
					free(mesonProp->prop_ptr->enums);
					mesonProp->prop_ptr->enums = NULL;
				}
				if (mesonProp->prop_ptr->blob_ids ) {
					free(mesonProp->prop_ptr->blob_ids);
					mesonProp->prop_ptr->blob_ids = NULL;
				}
				memcpy( mesonProp->prop_ptr,prop_temp, sizeof(drmModePropertyRes) );

				if (mesonProp->prop_ptr->count_enums > 0) {
					enums_temp = (struct drm_mode_property_enum*)calloc( mesonProp->prop_ptr->count_enums, sizeof(struct drm_mode_property_enum));
					if (enums_temp)
						memcpy( enums_temp, mesonProp->prop_ptr->enums, mesonProp->prop_ptr->count_enums*sizeof(struct drm_mode_property_enum) );
				}
				mesonProp->prop_ptr->enums = enums_temp;

				if ( mesonProp->prop_ptr->count_values > 0 ) {
					values_temp = (uint64_t*)calloc( mesonProp->prop_ptr->count_values, sizeof(uint64_t));
					if (values_temp)
						memcpy( values_temp, mesonProp->prop_ptr->values, mesonProp->prop_ptr->count_values*sizeof(uint64_t) );
				}
				mesonProp->prop_ptr->values = values_temp;

				if ( mesonProp->prop_ptr->count_blobs > 0 ) {
					blob_ids_temp = (uint32_t*)calloc( mesonProp->prop_ptr->count_blobs, sizeof(uint32_t));
					if (blob_ids_temp)
						memcpy(blob_ids_temp, mesonProp->prop_ptr->blob_ids, mesonProp->prop_ptr->count_blobs*sizeof(uint32_t));
				}
				mesonProp->prop_ptr->blob_ids = blob_ids_temp;
				mesonProp->mComponentId = mesonProp->mComponentId;
				mesonProp->mValue = props->prop_values[i];
				mesonProp->mType = mesonProp->prop_ptr->flags;
				mesonProp->drmObjectType = mesonProp->drmObjectType;
				ret = 0;
				break;
			}
			drmModeFreeProperty(prop_temp);
			prop_temp = NULL;
		}
	}
exit:
	drmModeFreeProperty(prop_temp);
	drmModeFreeObjectProperties(props);
	return ret;
}
int mesonPropertyGetId(struct mesonProperty* mesonProp)
{
	int ret = -1;
	if ( mesonProp ) {
		ret = mesonProp->prop_ptr->prop_id;
	} else {
		printf("\t mesonPropertyGetValue invalid parameter\n");
	}
	return ret;
}

uint64_t mesonPropertyGetValue(struct mesonProperty* mesonProp)
{
	uint64_t ret = 0;
	if ( mesonProp ) {
		ret = mesonProp->mValue;
	} else {
		printf("\t mesonPropertyGetValue invalid parameter\n");
	}
	return ret;
}

uint32_t mesonPropertyGetType(struct mesonProperty* mesonProp)
{
	uint64_t ret = 0;
	if ( mesonProp ) {
		ret = mesonProp->mType;
	} else {
		printf("\t mesonPropertyGetType invalid parameter\n");
	}
	return ret;
}

/*only updated cached value, not update to driver*/
int mesonPropertyDestroy(struct mesonProperty* mesonProperty)
{
	int ret = -1;
	if ( mesonProperty ) {
		if ( mesonProperty->prop_ptr) {
			if (mesonProperty->prop_ptr->blob_ids)
				free(mesonProperty->prop_ptr->blob_ids);
			drmModeFreeProperty(mesonProperty->prop_ptr);
			mesonProperty->prop_ptr = NULL;
		}
		free(mesonProperty);
		mesonProperty = NULL;
		ret  = 0;
	} else {
		printf("\t mesonPropertyDestroy invalid parameter\n");
	}
	return ret;
}
int mesonPropertyGetBlobData(struct mesonProperty* mseonProp, int drmFd, int* len, char** data)
{
	int ret = -1;
	int blob_id = -1;
	drmModePropertyBlobPtr blobProp =  NULL;
	char *blob_data = NULL;
	if (!mseonProp || !len ) {
		printf("\t mesonPropertyGetBlobData invalid parameter\n");
		goto exit;
	}
	if (!drm_property_type_is(mseonProp->prop_ptr, DRM_MODE_PROP_BLOB)) {
		printf("\t mesonPropertyGetBlobData invalid parameter is not a blob property!!!\n");
		goto exit;
	}
	if ( mseonProp && len ) {
		blob_id = mseonProp->mValue;
		blobProp = drmModeGetPropertyBlob(drmFd, blob_id);
		if (!blobProp) {
			goto exit;
		}
		*len = blobProp->length;
		blob_data = (char*)calloc( blobProp->length, sizeof(char));
		if (blob_data) {
			memcpy(blob_data, blobProp->data, blobProp->length);
			*data = blob_data;
		}
		else
			printf("\t mesonPropertyGetBlobData fail to alloc buffer\n");
		drmModeFreePropertyBlob(blobProp);
		ret = 0;
	}
exit:
	return ret;
}
int mesonPropertyGetRange(struct mesonProperty* mesonProp, uint64_t * min, uint64_t * max)
{
	int ret = -1;
	if ( !mesonProp || !min || !max ) {
		printf("\t mesonPropertyGetRange invalid parameter\n");
		goto exit;
	}
	if (drm_property_type_is(mesonProp->prop_ptr, DRM_MODE_PROP_SIGNED_RANGE) || drm_property_type_is(mesonProp->prop_ptr, DRM_MODE_PROP_RANGE)) {
		*min = mesonProp->prop_ptr->values[0];
		*max = mesonProp->prop_ptr->values[1];
		ret = 0;
	} else {
		printf("\t mesonPropertyGetRange invalid parameter，not a range property\n");
	}
exit:
	return ret;

}
int mesonPropertyGetEnumTypes(struct mesonProperty* mesonProp, int *len, struct drm_mode_property_enum** enumTypes)
{
	int ret = -1;
	if ( !mesonProp || !len || !enumTypes ) {
		printf("\t mesonPropertyGetEnumTypes invalid parameter\n");
		goto exit;
	}
	if ( !mesonProp->prop_ptr ) {
		printf("\t mesonPropertyGetEnumTypes invalid parameter\n");
		goto exit;
	}
	if (drm_property_type_is(mesonProp->prop_ptr, DRM_MODE_PROP_ENUM)) {
		*len = mesonProp->prop_ptr->count_enums;
		*enumTypes = mesonProp->prop_ptr->enums;
		ret = 0;
	} else {
		printf("\t mesonPropertyGetRange invalid parameter，not a enum property\n");
	}
exit:
	return ret;
}

void dump_property(struct mesonProperty* mesonProp,int drmFd)
{
	printf("\n dump_property \n");
	int i = 0;
	if (mesonProp) {
		printf("name:%s\n",mesonProp->prop_ptr->name);
		printf("mComponentId:%d\n",mesonProp->mComponentId);
		printf("drmObjectType:%d\n",mesonProp->drmObjectType);
		printf("prop id:%d\n",mesonPropertyGetId(mesonProp));
		printf("\t\tflags:");
		if (mesonProp->drmObjectType & DRM_MODE_PROP_PENDING)
			printf(" pending");
		if (mesonProp->drmObjectType & DRM_MODE_PROP_IMMUTABLE)
			printf(" immutable");
		if (drm_property_type_is(mesonProp->prop_ptr, DRM_MODE_PROP_SIGNED_RANGE))
			printf(" signed range");
		if (drm_property_type_is(mesonProp->prop_ptr, DRM_MODE_PROP_RANGE))
			printf(" range");
		if (drm_property_type_is(mesonProp->prop_ptr, DRM_MODE_PROP_ENUM))
			printf(" enum");
		if (drm_property_type_is(mesonProp->prop_ptr, DRM_MODE_PROP_BITMASK))
			printf(" bitmask");
		if (drm_property_type_is(mesonProp->prop_ptr, DRM_MODE_PROP_BLOB))
			printf(" blob");
		if (drm_property_type_is(mesonProp->prop_ptr, DRM_MODE_PROP_OBJECT))
			printf(" object");
		printf("\n");
		printf("mValue:%d\n",(int)mesonProp->mValue);
		if (drm_property_type_is(mesonProp->prop_ptr, DRM_MODE_PROP_SIGNED_RANGE) ||
			drm_property_type_is(mesonProp->prop_ptr, DRM_MODE_PROP_RANGE) ) {
			printf("\t\t range:");
			uint64_t min = 0;
			uint64_t max = 0;
			mesonPropertyGetRange(mesonProp, &min, &max);
			printf("min:%d max:%d\n", (int)min, (int)max);
			printf("\n");
		}
		if (drm_property_type_is(mesonProp->prop_ptr, DRM_MODE_PROP_ENUM)) {
			printf("\t\tenums:");
			int enumTypeLen = 0;
			struct drm_mode_property_enum** enumTypes = NULL;
			mesonPropertyGetEnumTypes(mesonProp, &enumTypeLen, enumTypes );
			printf("enumTypeLen:%d\n",enumTypeLen);
			for ( i=0; i<enumTypeLen; i++ ) {
				printf("%s=%llu",(*enumTypes)[i].name,(*enumTypes)[i].value);
			}
		}
		printf("\t\blob:");
		if (drm_property_type_is(mesonProp->prop_ptr, DRM_MODE_PROP_BLOB)) {
			int blob_len = 0;
			char** blob_data = NULL;
			blob_data = (char**)calloc( 1, sizeof(char**) );
			mesonPropertyGetBlobData(mesonProp, drmFd, &blob_len, blob_data);

			for (i = 0; i < blob_len; i++) {
				if (i % 16 == 0)
					printf("\n\t\t\t");
				printf("%x", (*blob_data)[i]);
			}
			printf("\n");
			free(blob_data);
		}
	}
}
