 /*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <linux/stddef.h>
#include <linux/types.h>
#include <errno.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

#include <xf86drm.h>

#include "meson_drm.h"
#include "meson_drmif.h"
#include "meson_drm_log.h"

/*
 * Create meson drm device object.
 * @fd: file descriptor to meson drm driver opened.
 *
 * if true, returen the device object else NULL.
 */
struct meson_device *meson_device_create(int fd, int render_fd)
{
    struct meson_device *dev;

    dev = calloc(sizeof(*dev), 1);
    if (!dev) {
        ERROR("%s %d failed to create device[%s].\n",__FUNCTION__,__LINE__,
                strerror(errno));
        return NULL;
    }
    dev->fd = fd;
    dev->render_fd = render_fd;
    return dev;
}

/*
 * Destroy meson drm device object
 *
 * @dev: meson drm device object.
 */
void meson_device_destroy(struct meson_device *dev)
{
    free(dev);
}

/*
 * Create a meson buffer object to meson drm device.
 *
 * @dev: meson drm device object.
 * @size: buffer size.
 * @flags: buffer allocation flag.
 *
 *  user can set one or more flags to allocation specified buffer object
 *
 *  if true, return a meson buffer object else NULL.
 */
struct meson_bo *meson_bo_create(struct meson_device *dev, size_t size,
		uint32_t flags, int alloc_only)
{
	int fd;
    struct meson_bo *bo;
    struct drm_meson_gem_create req = {
        .size = size,
        .flags = flags,
    };

    if (size == 0) {
        ERROR("%s %d invalid size.\n",__FUNCTION__,__LINE__);
        goto fail;
    }
    bo = calloc(sizeof(*bo), 1);
    INFO("%s %d meson_bo_create：%p\n",__FUNCTION__,__LINE__,bo);
    if (!bo) {
        ERROR("%s %d failed to create bo[%s].\n",__FUNCTION__,__LINE__,
            strerror(errno));
        goto err_free_bo;
    }

    bo->dev = dev;
    fd = alloc_only ? dev->render_fd : dev->fd;

    if (drmIoctl(fd, DRM_IOCTL_MESON_GEM_CREATE, &req)) {
        ERROR("%s %d failed to create gem object[%s].\n",__FUNCTION__,__LINE__,
                strerror(errno));
        goto err_free_bo;
    }

    bo->handle = req.handle;
    bo->size = size;
    bo->flags = flags;
    return bo;

err_free_bo:
    free(bo);
fail:
    return NULL;
}

/*
 * Destroy a meson buffer object.
 * @bo: a meson buffer object to be destroyed.
 */
void meson_bo_destroy(struct meson_bo *bo)
{
    INFO("%s %d meson_bo_destroy：%p\n",__FUNCTION__,__LINE__,bo);
    if (!bo)
        return;
    if (bo->vaddr)
        munmap(bo->vaddr, bo->size);
    if (bo->fd)
        close(bo->fd);
    if (bo->handle) {
        struct drm_gem_close req = {
            .handle = bo->handle,
        };
        drmIoctl(bo->dev->fd, DRM_IOCTL_GEM_CLOSE, &req);
    }
    free(bo);
}

uint32_t meson_bo_handle(struct meson_bo *bo)
{
    return bo->handle;
}

int meson_bo_dmabuf(struct meson_bo *bo, int alloc_only)
{
    int fd = alloc_only ? bo->dev->render_fd : bo->dev->fd;
    if (!bo->fd) {
        struct drm_prime_handle req = {
            .handle = bo->handle,
            .flags = DRM_CLOEXEC | DRM_RDWR,
        };

        int ret;

        ret = drmIoctl(fd, DRM_IOCTL_PRIME_HANDLE_TO_FD, &req);
        if (ret)
            return ret;

        bo->fd = req.fd;
    }

    return bo->fd;
}

size_t meson_bo_size(struct meson_bo *bo)
{
    return bo->size;
}

struct meson_bo *meson_bo_import(struct meson_device *dev, int fd, size_t size, uint32_t flags)
{
    int ret;
    struct meson_bo *bo = NULL;

    struct drm_prime_handle req = {
        .fd = fd,
    };

    bo = calloc(sizeof(*bo), 1);
    if (!bo) {
        ERROR("%s %d failed to create bo[%s].\n",__FUNCTION__,__LINE__,
            strerror(errno));
        goto fail;
    }

    ret = drmIoctl(dev->fd, DRM_IOCTL_PRIME_FD_TO_HANDLE, &req);
    if (ret)
        goto err_free_bo;

    bo->dev = dev;
    bo->size = size;
    bo->flags = flags;
    bo->handle = req.handle;

    return bo;

err_free_bo:
    free(bo);
fail:
    return NULL;
}
