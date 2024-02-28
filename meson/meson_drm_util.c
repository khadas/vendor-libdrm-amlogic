 /*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#include <poll.h>
#include <sys/ioctl.h>
#include "dma-buf.h"
#include "meson_drm_util.h"
#include "meson_drm_log.h"

struct drm_display *drm_kms_init(void);

struct drm_display *drm_display_init(void)
{
     return drm_kms_init();
}

void drm_destroy_display(struct drm_display *disp)
{
    disp->destroy_display(disp);
}

void drm_display_register_done_cb(struct drm_display *disp, void *func, void *priv)
{
    disp->display_done_callback = func;
    disp->priv = priv;
}

void drm_display_register_res_cb(struct drm_display *disp, void *func, void *priv)
{
    disp->resolution_change_callback = func;
    disp->reso_priv = priv;
}

int drm_set_alloc_only_flag(struct drm_display *disp, int flag)
{
    disp->alloc_only = flag;
}

int drm_alloc_bufs(struct drm_display *disp, int num, struct drm_buf_metadata *info)
{
    int ret;
    ret = disp->alloc_bufs(disp, num, info);

    return ret;
}

int drm_free_bufs(struct drm_display *disp)
{
    int ret;
    ret = disp->free_bufs(disp);

    return ret;
}

struct drm_buf *drm_alloc_buf(struct drm_display *disp, struct drm_buf_metadata *info)
{
    struct drm_buf *buf = NULL;

    buf = disp->alloc_buf(disp, info);

    return buf;
}

struct drm_buf *drm_import_buf(struct drm_display *disp, struct drm_buf_import *info)
{
    struct drm_buf *buf = NULL;

    buf = disp->import_buf(disp, info);

    return buf;
}

int drm_free_buf(struct drm_buf *buf)
{
    int ret;
    struct drm_display *disp = buf->disp;
    ret = disp->free_buf(buf);

    return ret;
}

int drm_post_buf(struct drm_display *disp, struct drm_buf *buf)
{
    return disp->post_buf(disp, buf);
}

int drm_waitvideoFence( int dmabuffd )
{
    struct dma_buf_export_sync_file dma_fence;
    int rc = -1;

    if (dmabuffd <= 0)
        return;

    memset (&dma_fence, 0, sizeof(dma_fence));
    dma_fence.flags |= DMA_BUF_SYNC_READ;
    rc = ioctl(dmabuffd, DMA_BUF_IOCTL_EXPORT_SYNC_FILE, &dma_fence);
    if (!rc && dma_fence.fd >= 0 )
    {
        struct pollfd pfd;

        pfd.fd= dma_fence.fd;
        pfd.events= POLLIN;
        pfd.revents= 0;

        for ( ; ; ) {
            rc= poll( &pfd, 1, 3000);
            if ( (rc == -1) && ((errno == EINTR) || (errno == EAGAIN)) )
            {
                continue;
            }
            else if ( rc <= 0 )
            {
               if ( rc == 0 ) errno= ETIME;
                ERROR("%s %d wait out video fence failed: fd %d errno %d\n",__FUNCTION__,__LINE__,
                    dma_fence.fd, errno);
            }
            else if(pfd.revents & (POLLNVAL | POLLERR)) {
                INFO("%s %d waiting on video fence fd %d, revents error\n",__FUNCTION__,__LINE__,
                    dma_fence.fd);
            }
            break;
        }
        close( dma_fence.fd );
        dma_fence.fd= -1;
    }

    return rc;
}
