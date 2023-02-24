 /*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stddef.h>
#include <drm_fourcc.h>
#include "meson_drm_util.h"


#define memclear(s) memset(&s, 0, sizeof(s))

#define U642VOID(x) ((void *)(unsigned long)(x))
#define VOID2U64(x) ((uint64_t)(unsigned long)(x))

static inline int DRM_IOCTL(int fd, unsigned long cmd, void *arg)
{
	int ret = drmIoctl(fd, cmd, arg);
	return ret < 0 ? -errno : ret;
}

typedef struct _drmModeAtomicReqItem drmModeAtomicReqItem, *drmModeAtomicReqItemPtr;

struct _drmModeAtomicReqItem {
	uint32_t object_id;
	uint32_t property_id;
	uint64_t value;
};

struct _drmModeAtomicReq {
	uint32_t cursor;
	uint32_t size_items;
	drmModeAtomicReqItemPtr items;
};

static int sort_req_list(const void *misc, const void *other)
{
	const drmModeAtomicReqItem *first = misc;
	const drmModeAtomicReqItem *second = other;

	if (first->object_id < second->object_id)
		return -1;
	else if (first->object_id > second->object_id)
		return 1;
	else
		return second->property_id - first->property_id;
}

int drmModeAsyncAtomicCommit(int fd, drmModeAtomicReqPtr req,
                                   uint32_t flags, void *user_data)
{
	drmModeAtomicReqPtr sorted;
	struct drm_mode_atomic atomic;
	uint32_t *objs_ptr = NULL;
	uint32_t *count_props_ptr = NULL;
	uint32_t *props_ptr = NULL;
	uint64_t *prop_values_ptr = NULL;
	uint32_t last_obj_id = 0;
	uint32_t i;
	int obj_idx = -1;
	int ret = -1;

	if (!req)
		return -EINVAL;

	if (req->cursor == 0)
		return 0;

	sorted = drmModeAtomicDuplicate(req);
	if (sorted == NULL)
		return -ENOMEM;

	memclear(atomic);

	/* Sort the list by object ID, then by property ID. */
	qsort(sorted->items, sorted->cursor, sizeof(*sorted->items),
	      sort_req_list);

	/* Now the list is sorted, eliminate duplicate property sets. */
	for (i = 0; i < sorted->cursor; i++) {
		if (sorted->items[i].object_id != last_obj_id) {
			atomic.count_objs++;
			last_obj_id = sorted->items[i].object_id;
		}

		if (i == sorted->cursor - 1)
			continue;

		if (sorted->items[i].object_id != sorted->items[i + 1].object_id ||
		    sorted->items[i].property_id != sorted->items[i + 1].property_id)
			continue;

		memmove(&sorted->items[i], &sorted->items[i + 1],
			(sorted->cursor - i - 1) * sizeof(*sorted->items));
		sorted->cursor--;
	}

	objs_ptr = drmMalloc(atomic.count_objs * sizeof objs_ptr[0]);
	if (!objs_ptr) {
		errno = ENOMEM;
		goto out;
	}

	count_props_ptr = drmMalloc(atomic.count_objs * sizeof count_props_ptr[0]);
	if (!count_props_ptr) {
		errno = ENOMEM;
		goto out;
	}

	props_ptr = drmMalloc(sorted->cursor * sizeof props_ptr[0]);
	if (!props_ptr) {
		errno = ENOMEM;
		goto out;
	}

	prop_values_ptr = drmMalloc(sorted->cursor * sizeof prop_values_ptr[0]);
	if (!prop_values_ptr) {
		errno = ENOMEM;
		goto out;
	}

	for (i = 0, last_obj_id = 0; i < sorted->cursor; i++) {
		if (sorted->items[i].object_id != last_obj_id) {
			obj_idx++;
			objs_ptr[obj_idx] = sorted->items[i].object_id;
			last_obj_id = objs_ptr[obj_idx];
		}

		count_props_ptr[obj_idx]++;
		props_ptr[i] = sorted->items[i].property_id;
		prop_values_ptr[i] = sorted->items[i].value;

	}

	atomic.flags = flags;
	atomic.objs_ptr = VOID2U64(objs_ptr);
	atomic.count_props_ptr = VOID2U64(count_props_ptr);
	atomic.props_ptr = VOID2U64(props_ptr);
	atomic.prop_values_ptr = VOID2U64(prop_values_ptr);
	atomic.user_data = VOID2U64(user_data);

	ret = DRM_IOCTL(fd, DRM_IOCTL_MESON_ASYNC_ATOMIC, &atomic);

out:
	drmFree(objs_ptr);
	drmFree(count_props_ptr);
	drmFree(props_ptr);
	drmFree(prop_values_ptr);
	drmModeAtomicFree(sorted);

	return ret;
}

