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
#include "meson_drm_log.h"

#define MAX_OSD_PLANE 6
#define MAX_VIDEO_PLANE 4
#define MAX_CONNECTOR 3
#define MAX_CRTC 3

struct kms_display;


#define VOID2U64(x) ((uint64_t)(unsigned long)(x))
#define container_of(ptr, type, member) \
    (type *)((char *)(ptr) - (char *) &((type *)0)->member)

#define to_kms_display(x) container_of(x, struct kms_display, base)

struct property {
    uint32_t id;
    uint64_t value;
};

struct crtc_state {
    uint32_t id;

    struct property mode_id;
    struct property active;
    struct property out_fence;
    struct property video_out_fence;
};

struct connector_state {
    uint32_t id;
    uint32_t crtc_mask;
    drmModeModeInfo mode;

    struct property crtc_id;
};

struct plane_state {
    uint32_t id;
    uint32_t crtc_mask;

    struct property crtc_id;
    struct property crtc_x;
    struct property crtc_y;
    struct property crtc_w;
    struct property crtc_h;
    struct property fb_id;
    struct property src_x;
    struct property src_y;
    struct property src_w;
    struct property src_h;
    struct property type;
    struct property in_fence_fd;
    struct property in_formats;
};

struct kms_display {
    struct drm_display base;

    int num_crtc;
    struct crtc_state *crtc_states[MAX_CRTC];
    int num_connector;
    struct connector_state *conn_states[MAX_CONNECTOR];
    int num_osd_plane;
    struct plane_state *osd_states[MAX_OSD_PLANE];
    int num_vid_plane;
    struct plane_state *vid_states[MAX_OSD_PLANE];

    int mode_set;
};

static void kms_destroy_display(struct drm_display *drm_disp)
{
    int i;
    struct kms_display *disp = to_kms_display(drm_disp);
    close(drm_disp->drm_fd);
    close(drm_disp->dev->render_fd);
    meson_device_destroy(drm_disp->dev);

    for (i = 0; i < MAX_CRTC; i++) {
        if (disp->crtc_states[i])
            free(disp->crtc_states[i]);
    }

    for (i = 0; i < MAX_CONNECTOR; i++) {
        if (disp->conn_states[i])
            free(disp->conn_states[i]);
    }

    for (i = 0; i < MAX_OSD_PLANE; i++) {
        if (disp->osd_states[i])
            free(disp->osd_states[i]);
    }

    for (i = 0; i < MAX_OSD_PLANE; i++) {
        if (disp->vid_states[i])
            free(disp->vid_states[i]);
    }

    free(disp);
}

static int free_buf(struct drm_display *drm_disp, struct drm_buf *buf)
{
    int i, fd, ret;
    struct drm_mode_destroy_dumb destroy_dumb;

    if (!buf)
        return -1;

    for ( i = 0; i < buf->nbo; i++)
        close(buf->fd[i]);

    fd = drm_disp->alloc_only ? drm_disp->dev->render_fd : drm_disp->dev->fd;
    if (drm_disp->freeze) {
        ret = drmIoctl(drm_disp->drm_fd, DRM_IOCTL_MESON_RMFB, &buf->fb_id);
        if (ret < 0) {
            ERROR("%s %d Unable to rmfb: %s\n",__FUNCTION__,__LINE__,
                    strerror(errno));
        }
    } else {
        drmModeRmFB(drm_disp->drm_fd, buf->fb_id);
    }
    memset(&destroy_dumb, 0, sizeof(destroy_dumb));

    for ( i = 0; i < buf->nbo; i++) {
        if (!buf->bos[i])
            continue;

        destroy_dumb.handle = meson_bo_handle(buf->bos[i]);

        ret = drmIoctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy_dumb);
        if (ret < 0) {
            /* If handle was from drmPrimeFDToHandle, then fd is connected
             * as render, we have to use drm_gem_close to release it.
             */
            if (errno == EACCES) {
                struct drm_gem_close close_req;
                close_req.handle = destroy_dumb.handle;
                ret = drmIoctl(fd, DRM_IOCTL_GEM_CLOSE, &close_req);
                if (ret < 0) {
                    ERROR("%s %d Unable to destroy buffer: %s\n",__FUNCTION__,__LINE__,
                            strerror(errno));
                    return -1;
                }
            }
        }
        free(buf->bos[i]);
        buf->bos[i] = NULL;
    }
    free(buf);
    buf = NULL;
    return 0;
}
static int kms_free_bufs(struct drm_display *drm_disp)
{
    int i;
    struct drm_buf *buf;
    for ( i = 0; i < drm_disp->nbuf; i++ ) {
        buf = &drm_disp->bufs[i];
        free_buf(drm_disp, buf);
    }
    return 0;
}

static int kms_free_buf(struct drm_buf *buf)
{
    free_buf(buf->disp, buf);
    return 0;
}

static struct meson_bo *alloc_bo(struct drm_display *drm_disp, struct drm_buf *buf, uint32_t bpp, uint32_t width,
                                 uint32_t height, uint32_t *bo_handle, uint32_t *pitch)
{
    uint32_t size;
    struct meson_bo *bo;

    size = width * height * bpp / 8;
    bo = meson_bo_create(drm_disp->dev, size, buf->flags, drm_disp->alloc_only);
    if (bo) {
        *bo_handle = meson_bo_handle(bo);
        *pitch = width * bpp / 8;
        return bo;
    } else {
        ERROR("%s %d meson_bo_create fail\n",__FUNCTION__,__LINE__);
        return NULL;
    }
}

static int alloc_bos(struct drm_display *drm_disp, struct drm_buf *buf, uint32_t *bo_handles)
{
    uint32_t width = buf->width;
    uint32_t height = buf->height;

    switch (buf->fourcc) {
    case DRM_FORMAT_ARGB8888:
        buf->nbo = 1;
        buf->bos[0] = alloc_bo(drm_disp, buf, 32, width, height, &bo_handles[0], &buf->pitches[0]);
        if (!buf->bos[0]) {
            ERROR("%s %d alloc_bo argb888 fail\n",__FUNCTION__,__LINE__);
            return -1;
        }

        buf->size = width * height * 4;
        buf->fd[0] = meson_bo_dmabuf(buf->bos[0], drm_disp->alloc_only);
        break;
    case DRM_FORMAT_UYVY:
    case DRM_FORMAT_YUYV:
        buf->nbo = 1;
        buf->bos[0] = alloc_bo(drm_disp, buf, 16, width, height, &bo_handles[0], &buf->pitches[0]);
        if (!buf->bos[0]) {
            ERROR("%s %d alloc_bo yuyv or uyvy fail\n",__FUNCTION__,__LINE__);
            return -1;
        }

        buf->size = width * height * 2;
        buf->fd[0] = meson_bo_dmabuf(buf->bos[0], drm_disp->alloc_only);
        buf->commit_to_video = 1;
        break;
    case DRM_FORMAT_NV12:
    case DRM_FORMAT_NV21:
        buf->nbo = 2;
        buf->bos[0] = alloc_bo(drm_disp, buf, 8, width, height, &bo_handles[0], &buf->pitches[0]);
        if (!buf->bos[0]) {
            ERROR("%s %d alloc_bo nv12 or nv21 fail\n",__FUNCTION__,__LINE__);
            return -1;
        }

        buf->fd[0] = meson_bo_dmabuf(buf->bos[0], drm_disp->alloc_only);

        buf->bos[1] = alloc_bo(drm_disp, buf, 16, width/2, height/2, &bo_handles[1], &buf->pitches[1]);
        if (!buf->bos[1]) {
            ERROR("%s %d alloc_bo argb888 fail\n",__FUNCTION__,__LINE__);
            return -1;
        }
        buf->fd[1] = meson_bo_dmabuf(buf->bos[1], drm_disp->alloc_only);
        buf->size = width * height * 3 / 2;
        buf->commit_to_video = 1;
        break;
    default:
        ERROR("%s %d unsupport format: 0x%08x\n",__FUNCTION__,__LINE__,buf->fourcc);
        break;
    }

    return 0;
}

static int add_framebuffer(int fd, struct drm_buf *buf, uint32_t *bo_handles, uint64_t modifier)
{
    int i, ret;
    uint64_t modifiers[4] = { 0, 0, 0, 0 };
    uint32_t flags = 0;
    uint32_t fb_id;

    for ( i = 0; i < buf->nbo; i++) {
        if (bo_handles[i] != 0 && modifier != DRM_FORMAT_MOD_NONE) {
            flags |= DRM_MODE_FB_MODIFIERS;
            modifiers[i] = modifier;
        }
    }

    ret = drmModeAddFB2WithModifiers(fd, buf->width, buf->height, buf->fourcc,
                bo_handles, buf->pitches, buf->offsets, modifiers, &fb_id, flags);
    if (ret < 0) {
        ERROR("%s %d Unable to add framebuffer for plane: %s\n",__FUNCTION__,__LINE__, strerror(errno));
        return -1;
    }

    buf->fb_id = fb_id;
    return 0;
}

static struct drm_buf *kms_alloc_buf(struct drm_display *drm_disp, struct drm_buf_metadata *info)
{
    int ret;
    struct drm_buf *buf = NULL;
    uint32_t bo_handles[4] = {0};

    buf = calloc(1, sizeof(*buf));
    buf->fourcc = info->fourcc;
    buf->width = info->width;
    buf->height = info->height;
    buf->flags = info->flags;
    buf->fence_fd = -1;
    buf->disable_plane = 0;
    buf->disp = drm_disp;

    ret = alloc_bos(drm_disp, buf, bo_handles);
    if (ret) {
        ERROR("%s %d alloc_bos fail\n",__FUNCTION__,__LINE__);
        free(buf);
        return NULL;
    }

    /*for non-root users, just need alloc buf and don't need to add framebuffer*/
    if (drm_disp->alloc_only)
        return buf;

    ret = add_framebuffer(drm_disp->drm_fd, buf, bo_handles, DRM_FORMAT_MOD_NONE);
    if (ret) {
        ERROR("%s %d add_framebuffer fail, call free_buf\n",__FUNCTION__,__LINE__);
        free_buf(drm_disp, buf);
        return NULL;
    }

    return buf;
}

static int kms_alloc_bufs(struct drm_display *drm_disp, int num, struct drm_buf_metadata *info)
{
    int i, ret;
    struct drm_buf *buf;
    uint32_t bo_handles[4] = {0};

    drm_disp->bufs = calloc(num, sizeof(*drm_disp->bufs));
    drm_disp->nbuf = num;

    for ( i = 0; i < num; i++) {
        buf = &drm_disp->bufs[i];
        buf->fourcc = info->fourcc;
        buf->width = info->width;
        buf->height = info->height;
        buf->flags = info->flags;
        buf->fence_fd = -1;
        buf->disable_plane = 0;
        buf->disp = drm_disp;

        if (!info->fourcc)
            buf->fourcc = DRM_FORMAT_ARGB8888;

        switch (buf->fourcc) {
        case DRM_FORMAT_ARGB8888:
            buf->nbo = 1;
            break;
        case DRM_FORMAT_UYVY:
        case DRM_FORMAT_YUYV:
            buf->nbo = 1;
            break;
        case DRM_FORMAT_NV12:
        case DRM_FORMAT_NV21:
            buf->nbo = 2;
            break;
        default:
            ERROR("%s %d unsupport format: 0x%08x\n",__FUNCTION__,__LINE__, buf->fourcc);
            break;
        }

        ret = alloc_bos(drm_disp, buf, bo_handles);
        if (ret) {
            ERROR("%s %d alloc_bos fail\n",__FUNCTION__,__LINE__);
            return -1;
        }

        ret = add_framebuffer(drm_disp->drm_fd, buf, bo_handles, DRM_FORMAT_MOD_NONE);
        if (ret) {
            ERROR("%s %d add_framebuffer fail\n",__FUNCTION__,__LINE__);
            free_buf(drm_disp, buf);
            return -1;
        }
    }
    return 0;
}

static struct drm_buf *kms_import_buf(struct drm_display *disp, struct drm_buf_import *info)
{
    int i;
    uint32_t size;
    struct drm_buf *buf = NULL;
    uint32_t bo_handles[4] = {0};

    buf = calloc(1, sizeof(*buf));
    buf->fourcc = info->fourcc;
    buf->width = info->width;
    buf->height = info->height;
    buf->flags = info->flags;
    buf->fence_fd = -1;
    buf->disable_plane = 0;
    buf->disp = disp;

    if (!info->fourcc)
        buf->fourcc = DRM_FORMAT_ARGB8888;

    switch (buf->fourcc) {
    case DRM_FORMAT_ARGB8888:
        buf->nbo = 1;
        break;
    case DRM_FORMAT_UYVY:
    case DRM_FORMAT_YUYV:
        buf->nbo = 1;
        break;
    case DRM_FORMAT_NV12:
    case DRM_FORMAT_NV21:
        buf->nbo = 2;
        break;
    default:
        ERROR("%s %d unsupport format: 0x%08x\n",__FUNCTION__,__LINE__,buf->fourcc);
        break;
    }

    for (i = 0; i < buf->nbo; i++) {
        if ( i == 0 )
            size = info->width * info->height;
        else
            size = info->width * info->height / 2;

        buf->size += size;
        buf->fd[i] = info->fd[i];
        buf->pitches[i] = buf->width;
        buf->bos[i] = meson_bo_import(disp->dev, info->fd[i], size, info->flags);
        if (!buf->bos[i]) {
            ERROR("%s %d meson_bo_import fail\n",__FUNCTION__,__LINE__);
            return NULL;
        }

        bo_handles[i] = meson_bo_handle(buf->bos[i]);
    }

    add_framebuffer(disp->drm_fd, buf, bo_handles, DRM_FORMAT_MOD_NONE);

    return buf;
}

static int kms_set_plane(struct drm_display *drm_disp, struct drm_buf *buf)
{
    int ret;
    uint32_t crtc_w, crtc_h;
    struct kms_display *disp = to_kms_display(drm_disp);

    struct plane_state *plane_state;
    struct crtc_state *crtc_state;
    struct connector_state *conn_state;

    conn_state = disp->conn_states[0];
    crtc_state = disp->crtc_states[0];

    if (buf->flags & MESON_USE_VD1)
        plane_state = disp->vid_states[0];
    else if (buf->flags & MESON_USE_VD2)
        plane_state = disp->vid_states[1];
    else
        plane_state = disp->osd_states[0];


    if (buf->crtc_w == 0)
        crtc_w = conn_state->mode.hdisplay;
    else
        crtc_w = buf->crtc_w;

    if (buf->crtc_h == 0)
        crtc_h = conn_state->mode.vdisplay;
    else
        crtc_h = buf->crtc_h;

    if (!drmIsMaster(drm_disp->drm_fd))
        drmSetMaster(drm_disp->drm_fd);

    if (drmModeSetPlane(drm_disp->drm_fd, plane_state->id, crtc_state->id, buf->fb_id,
                0, buf->crtc_x, buf->crtc_y, crtc_w, crtc_h,
                buf->src_x << 16, buf->src_y << 16, buf->src_w << 16, buf->src_h << 16)) {
        ERROR("failed to set plane: %s\n", strerror(errno));
        drmDropMaster(drm_disp->drm_fd);
        return -1;
    }
        drmDropMaster(drm_disp->drm_fd);

    return ret;
}

static int kms_post_buf(struct drm_display *drm_disp, struct drm_buf *buf)
{
    int ret;
    drmModeAtomicReqPtr request;
    uint32_t flags = DRM_MODE_ATOMIC_NONBLOCK;
    struct kms_display *disp = to_kms_display(drm_disp);

    struct plane_state *plane_state;
    struct crtc_state *crtc_state;
    struct connector_state *conn_state;

    conn_state = disp->conn_states[0];
    crtc_state = disp->crtc_states[0];

    if (buf->flags & MESON_USE_VD1)
        plane_state = disp->vid_states[0];
    else if (buf->flags & MESON_USE_VD2)
        plane_state = disp->vid_states[1];
    else
        plane_state = disp->osd_states[0];

    request = drmModeAtomicAlloc();

    if (buf->disable_plane) {
        drmModeAtomicAddProperty(request, plane_state->id, plane_state->crtc_x.id, 0);
        drmModeAtomicAddProperty(request, plane_state->id, plane_state->crtc_y.id, 0);
        drmModeAtomicAddProperty(request, plane_state->id, plane_state->crtc_w.id, 0);
        drmModeAtomicAddProperty(request, plane_state->id, plane_state->crtc_h.id, 0);
        drmModeAtomicAddProperty(request, plane_state->id, plane_state->src_x.id, 0);
        drmModeAtomicAddProperty(request, plane_state->id, plane_state->src_y.id, 0);
        drmModeAtomicAddProperty(request, plane_state->id, plane_state->src_w.id, 0);
        drmModeAtomicAddProperty(request, plane_state->id, plane_state->src_h.id, 0);
        drmModeAtomicAddProperty(request, plane_state->id, plane_state->fb_id.id, 0);
        drmModeAtomicAddProperty(request, plane_state->id, plane_state->crtc_id.id, 0);
    } else {
    #if 0
        if (!disp->mode_set) {
            flags |= DRM_MODE_ATOMIC_ALLOW_MODESET;
            drmModeAtomicAddProperty(request, conn_state->id, conn_state->crtc_id.id, crtc_state->id);

            if (drmModeCreatePropertyBlob(drm_disp->drm_fd, &disp->conn_states[0]->mode,
                                    sizeof(drmModeModeInfo), &blob_id) != 0 ) {
                return -1;
            }
            drmModeAtomicAddProperty(request, crtc_state->id, crtc_state->mode_id.id, blob_id);
            drmModeAtomicAddProperty(request, crtc_state->id, crtc_state->active.id, 1);

            disp->mode_set = 1;
        }
    #else
        /*No modeset needed in post buf, modeset will control by systemservice.*/
    #endif

        drmModeAtomicAddProperty(request, plane_state->id, plane_state->crtc_x.id, buf->crtc_x);
        drmModeAtomicAddProperty(request, plane_state->id, plane_state->crtc_y.id, buf->crtc_y);
        if (buf->crtc_w == 0) {
            drmModeAtomicAddProperty(request, plane_state->id, plane_state->crtc_w.id, conn_state->mode.hdisplay);
        } else {
            drmModeAtomicAddProperty(request, plane_state->id, plane_state->crtc_w.id, buf->crtc_w);
        }

        if (buf->crtc_h == 0) {
            drmModeAtomicAddProperty(request, plane_state->id, plane_state->crtc_h.id, conn_state->mode.vdisplay);
        } else {
            drmModeAtomicAddProperty(request, plane_state->id, plane_state->crtc_h.id, buf->crtc_h);
        }

        drmModeAtomicAddProperty(request, plane_state->id, plane_state->src_x.id, buf->src_x << 16);
        drmModeAtomicAddProperty(request, plane_state->id, plane_state->src_y.id, buf->src_y << 16);
        drmModeAtomicAddProperty(request, plane_state->id, plane_state->src_w.id, buf->src_w << 16);
        drmModeAtomicAddProperty(request, plane_state->id, plane_state->src_h.id, buf->src_h << 16);
        drmModeAtomicAddProperty(request, plane_state->id, plane_state->fb_id.id, buf->fb_id);
        drmModeAtomicAddProperty(request, plane_state->id, plane_state->crtc_id.id, crtc_state->id);
    #if 0
        if (buf->flags | (MESON_USE_VD2 | MESON_USE_VD1))
            drmModeAtomicAddProperty(request, crtc_state->id, crtc_state->video_out_fence.id, VOID2U64(&buf->fence_fd));
        else
            drmModeAtomicAddProperty(request, crtc_state->id, crtc_state->out_fence.id, VOID2U64(&buf->fence_fd));
    #endif
    }

    ret = drmModeAsyncAtomicCommit(drm_disp->drm_fd, request, flags, NULL);
    if (ret < 0) {
        ERROR("%s %d Unable to flip page: %s\n",__FUNCTION__,__LINE__,strerror(errno));
        goto error;
    }

    ret = 0;
    goto complete;

error:
    ret = -1;
complete:
    drmModeAtomicFree(request);

    return ret;
}

static void getproperty(int drm_fd, drmModeObjectProperties* props, const char *name,
                        struct property *p)
{
    int i;
    drmModePropertyRes *res;
    for (i = 0; i < props->count_props; i++) {
        res = drmModeGetProperty(drm_fd, props->props[i]);
        if (res && !strcmp(name, res->name)) {
            p->id = res->prop_id;
            p->value = props->prop_values[i];
            //fprintf(stdout, "getproperty: %s, id: %u, value: %llu \n", res->name, p->id, p->value);
        }
        drmModeFreeProperty(res);
    }
}

static int populate_connectors(drmModeRes *resources, struct kms_display *disp)
{
    int i, j;
    int num_connector = 0;
    struct connector_state *state;
    drmModeConnector *connector;
    drmModeEncoder *encoder;
    drmModeObjectProperties *props;

    for (i = 0; i < resources->count_connectors; i++) {
        connector = drmModeGetConnector(disp->base.drm_fd, resources->connectors[i]);
        if (!connector) {
            ERROR("%s %d drmModeGetConnector fail.\n",__FUNCTION__,__LINE__);
            continue;
        }

        if (connector->connector_type == DRM_MODE_CONNECTOR_TV) {
            drmModeFreeConnector(connector);
            continue;
        }
        state = calloc(1, sizeof(*state));
        disp->conn_states[num_connector++] = state;
        state->id = resources->connectors[i];

        for (j = 0; j < connector->count_encoders; j++) {
            encoder = drmModeGetEncoder(disp->base.drm_fd, connector->encoders[j]);
            if (encoder) {
                state->crtc_mask |= encoder->possible_crtcs;
                drmModeFreeEncoder(encoder);
            }
        }

        if (connector->count_modes)
            state->mode = connector->modes[0];

        for (j = 0; j < connector->count_modes; j++) {
            if (connector->modes[j].type & DRM_MODE_TYPE_PREFERRED) {
                state->mode = connector->modes[j];
                break;
            }
        }

        disp->base.width = state->mode.hdisplay;
        disp->base.height = state->mode.vdisplay;
        disp->base.vrefresh = state->mode.vrefresh;

        props = drmModeObjectGetProperties(disp->base.drm_fd, resources->connectors[i], DRM_MODE_OBJECT_CONNECTOR);
        getproperty(disp->base.drm_fd, props, "CRTC_ID", &state->crtc_id);

        if (props)
            drmModeFreeObjectProperties(props);

        if (connector)
            drmModeFreeConnector(connector);
    }

    disp->num_connector = num_connector;
    INFO("%s %d found %d connector\n",__FUNCTION__,__LINE__,num_connector);
    return 0;
}

static int populate_crtcs(drmModeRes *resources, struct kms_display *disp)
{
    int i;
    int num_crtc = 0;
    struct crtc_state *state;
    drmModeObjectProperties *props;

    for (i = 0; i < resources->count_crtcs; i++) {
        state = calloc(1, sizeof(*state));
        if (!state) {
            ERROR("%s %d calloc crtc state fail.\n",__FUNCTION__,__LINE__);
            return -1;
        }

        disp->crtc_states[num_crtc++] = state;
        state->id = resources->crtcs[i];

        props = drmModeObjectGetProperties(disp->base.drm_fd, resources->crtcs[i], DRM_MODE_OBJECT_CRTC);
        if (props) {
            getproperty(disp->base.drm_fd, props, "MODE_ID", &state->mode_id);
            getproperty(disp->base.drm_fd, props, "ACTIVE", &state->active);
            getproperty(disp->base.drm_fd, props, "OUT_FENCE_PTR", &state->out_fence);
            getproperty(disp->base.drm_fd, props, "VIDEO_OUT_FENCE_PTR", &state->video_out_fence);
            drmModeFreeObjectProperties(props);
        } else {
            ERROR("%s %d get crtc obj property fail\n",__FUNCTION__,__LINE__);
            return -1;
        }
    }

    disp->num_crtc = num_crtc;

    DEBUG("%s %d found %d crtc\n",__FUNCTION__,__LINE__,num_crtc);
    return 0;
}

static int is_plane_support_yuv(drmModePlane * plane)
{
    int i;
    for (i = 0; i < plane->count_formats; i++) {
        switch (plane->formats[i]) {
        case DRM_FORMAT_NV12:
        case DRM_FORMAT_NV21:
            return 1;
        default:
            return 0;
        }
    }

    return 0;
}

static int populate_planes(drmModeRes *resources, struct kms_display *disp)
{
    int i;
    int num_osd_plane = 0;
    int num_vid_plane = 0;
    struct plane_state *state;
    drmModePlane *plane;
    drmModePlaneRes *plane_res;
    drmModeObjectProperties *props;

    plane_res = drmModeGetPlaneResources(disp->base.drm_fd);
    if (!plane_res) {
        ERROR("%s %d drmModeGetPlaneResources fail.\n",__FUNCTION__,__LINE__);
        goto error;
    }

    for (i = 0; i < plane_res->count_planes; i++) {
        state = calloc(1, sizeof(*state));
        if (!state) {
            ERROR("%s %d calloc plane state fail.\n",__FUNCTION__,__LINE__);
            goto error;
        }

        plane = drmModeGetPlane(disp->base.drm_fd, plane_res->planes[i]);
        if (plane) {
            if (is_plane_support_yuv(plane))
                disp->vid_states[num_vid_plane++] = state;
            else
                disp->osd_states[num_osd_plane++] = state;

            state->id = plane_res->planes[i];
            state->crtc_mask = plane->possible_crtcs;

            props = drmModeObjectGetProperties(disp->base.drm_fd, plane_res->planes[i], DRM_MODE_OBJECT_PLANE);
            if (props) {
                getproperty(disp->base.drm_fd, props, "CRTC_ID", &state->crtc_id);
                getproperty(disp->base.drm_fd, props, "CRTC_X", &state->crtc_x);
                getproperty(disp->base.drm_fd, props, "CRTC_Y", &state->crtc_y);
                getproperty(disp->base.drm_fd, props, "CRTC_W", &state->crtc_w);
                getproperty(disp->base.drm_fd, props, "CRTC_H", &state->crtc_h);
                getproperty(disp->base.drm_fd, props, "FB_ID", &state->fb_id);
                getproperty(disp->base.drm_fd, props, "SRC_X", &state->src_x);
                getproperty(disp->base.drm_fd, props, "SRC_Y", &state->src_y);
                getproperty(disp->base.drm_fd, props, "SRC_W", &state->src_w);
                getproperty(disp->base.drm_fd, props, "SRC_H", &state->src_h);
                getproperty(disp->base.drm_fd, props, "type", &state->type);
                getproperty(disp->base.drm_fd, props, "IN_FENCE_FD", &state->in_fence_fd);
                getproperty(disp->base.drm_fd, props, "IN_FORMATS", &state->in_formats);
            }
        }

        if (props)
            drmModeFreeObjectProperties(props);

        if (plane)
            drmModeFreePlane(plane);
    }

    disp->num_osd_plane = num_osd_plane;
    disp->num_vid_plane = num_vid_plane;

    drmModeFreePlaneResources(plane_res);

    INFO("%s %d found %d osd, %d video\n",__FUNCTION__,__LINE__,num_osd_plane, num_vid_plane);
    return 0;

error:
    drmModeFreePlaneResources(plane_res);

    return -1;
}

static int drm_kms_init_resource(struct kms_display *disp)
{
    int ret;
    int drm_fd = -1;
    int render_fd = -1;
    drmModeRes *resources;

    drm_fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
    if (drm_fd < 0) {
        ERROR("%s %d Unable to open DRM node: %s\n",__FUNCTION__,__LINE__,
               strerror(errno));
        return -1;
    }
    drmDropMaster(drm_fd);
    render_fd = open("/dev/dri/renderD128", O_RDWR | O_CLOEXEC);
    if (render_fd < 0) {
        ERROR("%s %d Unable to open renderD128 node: %s\n",__FUNCTION__,__LINE__,
               strerror(errno));
        goto error3;
    }

    disp->base.drm_fd = drm_fd;
    disp->base.dev = meson_device_create(drm_fd, render_fd);
    if (!disp->base.dev) {
        ERROR("%s %d meson_device_create fail\n",__FUNCTION__,__LINE__);
        goto error3;
    }

    ret = drmSetClientCap(drm_fd, DRM_CLIENT_CAP_ATOMIC, 1);
    if (ret < 0) {
        ERROR("%s %d Unable to set DRM atomic capability: %s\n",__FUNCTION__,__LINE__,
                strerror(errno));
        goto error2;
    }

    ret = drmSetClientCap(drm_fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);
    if (ret < 0) {
        ERROR("%s %d Unable to set DRM universal planes capability: %s\n",__FUNCTION__,__LINE__,
                strerror(errno));
        goto error2;
    }

    resources = drmModeGetResources(drm_fd);
    if (!resources) {
        ERROR("%s %d drmModeGetResources failed: %s\n",__FUNCTION__,__LINE__, strerror(errno));
        goto error2;
    }

    ret = populate_connectors(resources, disp);
    if (ret)
        goto error1;

    ret = populate_crtcs(resources, disp);
    if (ret)
        goto error1;

    ret = populate_planes(resources, disp);
    if (ret)
        goto error1;
    drmModeFreeResources(resources);
    return 0;

error1:
    drmModeFreeResources(resources);
error2:
    meson_device_destroy(disp->base.dev);
error3:
    if (drm_fd >= 0)
        close(drm_fd);
    if (render_fd >= 0)
        close(render_fd);

    return -1;
}

struct drm_display *drm_kms_init(void)
{
    int ret;
    struct kms_display *display;
    struct drm_display *base;
    display = calloc(1, sizeof(*display));
    if (!display) {
        ERROR("%s %d calloc kms_display fail\n",__FUNCTION__,__LINE__);
        return NULL;
    }

    base = &display->base;
    base->destroy_display = kms_destroy_display;
    base->alloc_bufs = kms_alloc_bufs;
    base->free_bufs = kms_free_bufs;

    base->alloc_buf = kms_alloc_buf;
    base->import_buf = kms_import_buf;
    base->free_buf = kms_free_buf;
    base->post_buf = kms_post_buf;
    base->set_plane = kms_set_plane;
    base->alloc_only = 0;
    base->freeze = 0;

    ret = drm_kms_init_resource(display);
    if (ret) {
        ERROR("%s %d drm_kms_init_resource fail.\n",__FUNCTION__,__LINE__);
        free(display);
        return NULL;
    }

    return base;
}
