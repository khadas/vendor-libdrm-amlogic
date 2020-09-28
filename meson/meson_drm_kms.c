/*
 * Copyright (C) 2020 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stddef.h>
#include <drm_fourcc.h>
#include "meson_drm_util.h"

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

struct plane {
    drmModePlane *plane;
    drmModeObjectProperties *props;
    drmModePropertyRes **props_info;
};

struct crtc {
    drmModeCrtc *crtc;
    drmModeObjectProperties *props;
    drmModePropertyRes **props_info;
};

struct connector {
    drmModeConnector *connector;
    drmModeObjectProperties *props;
    drmModePropertyRes **props_info;
};

struct kms_display {
    struct drm_display base;

    struct plane *plane;
    struct crtc *crtc;
    struct connector *conn;

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
    struct kms_display *disp = to_kms_display(drm_disp);
    free(disp->conn);
    free(disp->crtc);
    free(disp->plane);
    free(disp);
}

static int alloc_bos(struct drm_display *drm_disp, struct drm_buf *buf, uint32_t *bo_handles)
{
    int i;
    uint32_t size = 0;
    uint32_t width = buf->width;
    uint32_t height = buf->height;

    for (i = 0; i < buf->nbo; i++) {
        if (buf->nbo == 1 && buf->flags == MESON_USE_VIDEO_AFBC) {
            size = width * height;
            buf->pitches[i] = width * 2;
            buf->commit_to_video = 1;
        } else if (buf->flags | MESON_USE_VIDEO_PLANE) {
            if ( i == 0 )
                size = width * height;
            else
                size = width * height / 2;

            buf->pitches[i] = width;
            buf->commit_to_video = 1;
        }

        buf->size += size;
        buf->bos[i] = meson_bo_create(drm_disp->dev, size, buf->flags);
        if (buf->bos[i]) {
            bo_handles[i] = meson_bo_handle(buf->bos[i]);
            buf->fd[i] = meson_bo_dmabuf(buf->bos[i]);
        } else {
            fprintf(stderr, "meson_bo_create fila\n");
            return -1;
        }
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
        fprintf(stderr, "Unable to add framebuffer for plane: %s\n", strerror(errno));
		return -1;
    }

    buf->fb_id = fb_id;
    return 0;
}

static struct drm_buf *kms_alloc_buf(struct drm_display *drm_disp, struct drm_buf_metadata *info)
{
    struct drm_buf *buf = NULL;
    uint32_t bo_handles[4] = {0};

    buf = calloc(1, sizeof(*buf));
    buf->fourcc = info->fourcc;
    buf->width = info->width;
    buf->height = info->height;
    buf->flags = info->flags;
    buf->fence_fd = -1;
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
        fprintf(stderr, "unsupport format: 0x%08x\n", buf->fourcc);
        break;
    }

    alloc_bos(drm_disp, buf, bo_handles);

    add_framebuffer(drm_disp->drm_fd, buf, bo_handles, DRM_FORMAT_MOD_NONE);

    return buf;
}
static int kms_alloc_bufs(struct drm_display *drm_disp, int num, struct drm_buf_metadata *info)
{
    int i;
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
            fprintf(stderr, "unsupport format: 0x%08x\n", buf->fourcc);
            break;
        }

        alloc_bos(drm_disp, buf, bo_handles);

        add_framebuffer(drm_disp->drm_fd, buf, bo_handles, DRM_FORMAT_MOD_NONE);
    }
    return 0;
}

static int free_buf(struct drm_display *drm_disp, struct drm_buf *buf)
{
    int i, ret;
    struct drm_mode_destroy_dumb destroy_dumb;

    for ( i = 0; i < buf->nbo; i++)
        close(buf->fd[i]);

    drmModeRmFB(drm_disp->drm_fd, buf->fb_id);
    memset(&destroy_dumb, 0, sizeof(destroy_dumb));

    for ( i = 0; i < buf->nbo; i++) {
        destroy_dumb.handle = meson_bo_handle(buf->bos[i]);

        ret = drmIoctl(drm_disp->drm_fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy_dumb);
        if (ret < 0) {
            /* If handle was from drmPrimeFDToHandle, then fd is connected
             * as render, we have to use drm_gem_close to release it.
             */
            if (errno == EACCES) {
                struct drm_gem_close close_req;
                close_req.handle = destroy_dumb.handle;
                ret = drmIoctl(drm_disp->drm_fd, DRM_IOCTL_GEM_CLOSE, &close_req);
                if (ret < 0) {
                    fprintf(stderr, "Unable to destroy buffer: %s\n",
                            strerror(errno));
                    return -1;
                }
            }
        }
    }

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

static int kms_post_buf(struct drm_display *drm_disp, struct drm_buf *buf)
{
    int ret;
    drmModeAtomicReqPtr request;
    uint32_t blob_id;
    uint32_t flags = DRM_MODE_ATOMIC_NONBLOCK;
    struct kms_display *disp = to_kms_display(drm_disp);

    struct plane_state *plane_state;
    struct crtc_state *crtc_state;
    struct connector_state *conn_state;

    conn_state = disp->conn_states[0];
    crtc_state = disp->crtc_states[0];

    if (buf->flags | MESON_USE_VD1)
        plane_state = disp->vid_states[0];
    else if (buf->flags | MESON_USE_VD2)
        plane_state = disp->vid_states[1];
    else
        plane_state = disp->osd_states[0];

    request = drmModeAtomicAlloc();

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

    drmModeAtomicAddProperty(request, plane_state->id, plane_state->crtc_x.id, 0);
    drmModeAtomicAddProperty(request, plane_state->id, plane_state->crtc_y.id, 0);
    drmModeAtomicAddProperty(request, plane_state->id, plane_state->crtc_w.id, conn_state->mode.hdisplay);
    drmModeAtomicAddProperty(request, plane_state->id, plane_state->crtc_h.id, conn_state->mode.vdisplay);
    drmModeAtomicAddProperty(request, plane_state->id, plane_state->src_x.id, 0);
    drmModeAtomicAddProperty(request, plane_state->id, plane_state->src_y.id, 0);
    drmModeAtomicAddProperty(request, plane_state->id, plane_state->src_w.id, buf->width << 16);
    drmModeAtomicAddProperty(request, plane_state->id, plane_state->src_h.id, buf->height << 16);
    drmModeAtomicAddProperty(request, plane_state->id, plane_state->fb_id.id, buf->fb_id);
    drmModeAtomicAddProperty(request, plane_state->id, plane_state->crtc_id.id, crtc_state->id);
#if 0
    if (buf->flags | (MESON_USE_VD2 | MESON_USE_VD1))
        drmModeAtomicAddProperty(request, crtc_state->id, crtc_state->video_out_fence.id, VOID2U64(&buf->fence_fd));
    else
        drmModeAtomicAddProperty(request, crtc_state->id, crtc_state->out_fence.id, VOID2U64(&buf->fence_fd));
#endif
    ret = drmModeAtomicCommit(drm_disp->drm_fd, request, flags, NULL);
    if (ret < 0) {
        fprintf(stderr, "Unable to flip page: %s\n", strerror(errno));
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
        if (strcmp(name, res->name))
            continue;

        p->id = res->prop_id;
        p->value = props->prop_values[i];
        //fprintf(stdout, "getproperty: %s, id: %u, value: %llu \n", res->name, p->id, p->value);
    }
}

static int populate_connectors(drmModeRes *resources, struct kms_display *disp)
{
    int ret, i, j;
    int num_connector = 0;
    struct connector_state *state;
    drmModeConnector *connector;
    drmModeEncoder *encoder;
    drmModeObjectProperties *props;

    for (i = 0; i < resources->count_connectors; i++) {
        connector = drmModeGetConnector(disp->base.drm_fd, resources->connectors[i]);
        disp->conn[i].connector = connector;

        if ( connector->connector_type == DRM_MODE_CONNECTOR_TV )
            continue;

        state = calloc(1, sizeof(*state));
        disp->conn_states[num_connector++] = state;
        state->id = resources->connectors[i];

        for (j = 0; j < connector->count_encoders; j++) {
            encoder = drmModeGetEncoder(disp->base.drm_fd, connector->encoders[j]);
            state->crtc_mask |= encoder->possible_crtcs;
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
        disp->conn[i].props = props;
        getproperty(disp->base.drm_fd, props, "CRTC_ID", &state->crtc_id);
    }

    disp->num_connector = num_connector;
    fprintf(stdout, "found %d connector\n", num_connector);
}

static int populate_crtcs(drmModeRes *resources, struct kms_display *disp)
{
    int ret, i;
    int num_crtc = 0;
    struct crtc_state *state;
    drmModeObjectProperties *props;

    for (i = 0; i < resources->count_crtcs; i++) {
        state = calloc(1, sizeof(*state));
        disp->crtc_states[num_crtc++] = state;
        state->id = resources->crtcs[i];

        props = drmModeObjectGetProperties(disp->base.drm_fd, resources->crtcs[i], DRM_MODE_OBJECT_CRTC);
        disp->crtc[i].props = props;
        getproperty(disp->base.drm_fd, props, "MODE_ID", &state->mode_id);
        getproperty(disp->base.drm_fd, props, "ACTIVE", &state->active);
        getproperty(disp->base.drm_fd, props, "OUT_FENCE_PTR", &state->out_fence);
        getproperty(disp->base.drm_fd, props, "VIDEO_OUT_FENCE_PTR", &state->video_out_fence);
    }

    disp->num_crtc = num_crtc;

    fprintf(stdout, "found %d crtc\n", num_crtc);
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
    int ret, i;
    int num_osd_plane = 0;
    int num_vid_plane = 0;
    struct plane_state *state;
    drmModePlane *plane;
    drmModePlaneRes *plane_res;
    drmModeObjectProperties *props;

    plane_res = drmModeGetPlaneResources(disp->base.drm_fd);

    disp->plane = calloc(plane_res->count_planes, sizeof(*disp->plane));

    for (i = 0; i < plane_res->count_planes; i++) {
        state = calloc(1, sizeof(*state));
        plane = drmModeGetPlane(disp->base.drm_fd, plane_res->planes[i]);
        disp->plane[i].plane = plane;
        if (is_plane_support_yuv(plane))
            disp->vid_states[num_vid_plane++] = state;
        else
            disp->osd_states[num_osd_plane++] = state;

        state->id = plane_res->planes[i];
        state->crtc_mask = plane->possible_crtcs;

        props = drmModeObjectGetProperties(disp->base.drm_fd, plane_res->planes[i], DRM_MODE_OBJECT_PLANE);
        disp->plane[i].props = props;
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

    disp->num_osd_plane = num_osd_plane;
    disp->num_vid_plane = num_vid_plane;

    fprintf(stdout, "found %d osd, %d video\n", num_osd_plane, num_vid_plane);
}

static int drm_kms_init_resource(struct kms_display *disp)
{
    int ret;
    int drm_fd;
    drmModeRes *resources;

    drm_fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
    if (drm_fd < 0) {
        fprintf(stderr, "Unable to open DRM node: %s\n",
               strerror(errno));
        return -1;
    }

    disp->base.drm_fd = drm_fd;
    disp->base.dev = meson_device_create(drm_fd);

    ret = drmSetClientCap(drm_fd, DRM_CLIENT_CAP_ATOMIC, 1);
    if (ret < 0) {
        fprintf(stderr, "Unable to set DRM atomic capability: %s\n",
                strerror(errno));
        return -1;
    }

    ret = drmSetClientCap(drm_fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);
    if (ret < 0) {
        fprintf(stderr, "Unable to set DRM universal planes capability: %s\n",
                strerror(errno));
        return -1;
    }

    resources = drmModeGetResources(drm_fd);
    if (!resources) {
        fprintf(stderr, "drmModeGetResources failed: %s\n", strerror(errno));
        return -1;
    }

    disp->conn = calloc(resources->count_connectors, sizeof(*disp->conn));
    disp->crtc = calloc(resources->count_crtcs, sizeof(*disp->crtc));

    populate_connectors(resources, disp);
    populate_crtcs(resources, disp);
    populate_planes(resources, disp);

    return 0;
}

struct drm_display *drm_kms_init(void)
{
    struct kms_display *display;
    struct drm_display *base;
    display = calloc(1, sizeof(*display));

    base = &display->base;
    base->destroy_display = kms_destroy_display;
    base->alloc_bufs = kms_alloc_bufs;
    base->free_bufs = kms_free_bufs;

    base->alloc_buf = kms_alloc_buf;
    base->free_buf = kms_free_buf;
    base->post_buf = kms_post_buf;

    drm_kms_init_resource(display);

    return base;
}
