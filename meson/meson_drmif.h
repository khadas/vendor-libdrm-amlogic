 /*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef _MESON_DRMIF_H_
#define _MESON_DRMIF_H_

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <xf86drm.h>

#include "meson_drm.h"

struct meson_device {
    int fd;
	int render_fd;
};

struct meson_bo {
    struct meson_device *dev;
    uint32_t handle;
    uint32_t flags;
    int fd;
    size_t size;
    void *vaddr;
};

/*
 * device related functions:
 */
struct meson_device *meson_device_create(int fd, int render_fd);
void meson_device_destroy(struct meson_device *dev);

/*
 * buffer-object related functions:
 */
struct meson_bo *meson_bo_create(struct meson_device *dev, size_t size, uint32_t flags, int alloc_only);
void meson_bo_destroy(struct meson_bo *bo);
uint32_t meson_bo_handle(struct meson_bo *bo);
int meson_bo_dmabuf(struct meson_bo *bo, int alloc_only);
size_t meson_bo_size(struct meson_bo *bo);
struct meson_bo *meson_bo_import(struct meson_device *dev, int fd, size_t size, uint32_t flags);

#endif
