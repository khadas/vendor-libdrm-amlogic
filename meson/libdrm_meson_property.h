 /*
 * Copyright (c) 2021 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef LIBDRM_MESON_PROPERTY_H_
#define LIBDRM_MESON_PROPERTY_H_
#if defined(__cplusplus)
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <xf86drmMode.h>
struct mesonProperty;
struct mesonProperty *mesonPropertyCreate(int drmFd, uint32_t drmObject, uint32_t drmObjectType, char * name);
/*update cached value*/
int mesonPropertyUpdateValue(int drmFd, struct mesonProperty* mesonProp);
int mesonPropertyGetId(struct mesonProperty* mesonProp );
uint64_t mesonPropertyGetValue(struct mesonProperty* mesonProp);
uint32_t mesonPropertyGetType(struct mesonProperty* mesonProp);
/*only updated cached value, not update to driver*/
int mesonPropertyDestroy(struct mesonProperty* mesonProp);
int mesonPropertyGetBlobData(struct mesonProperty* mseonProp, int drmFd, int* len, char** data);
int mesonPropertyGetRange(struct mesonProperty *, uint64_t* min, uint64_t* max);
int mesonPropertyGetEnumTypes(struct mesonProperty* mesonProp, int * len, struct drm_mode_property_enum** enumTypes);
#if defined(__cplusplus)
}
#endif

#endif
