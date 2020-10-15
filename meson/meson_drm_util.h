/*
 * Copyright (C) 2020 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#ifndef _MESON_DRM_UTIL_H_
#define _MESON_DRM_UTIL_H_

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <xf86drmMode.h>
#include "meson_drmif.h"

struct drm_buf {
    uint32_t fourcc;
    uint32_t width;
    uint32_t height;
    uint32_t flags;
    uint32_t size;
    int bpp;
    int nbo;
    struct meson_bo *bos[4];
    uint32_t pitches[4];
    uint32_t offsets[4];
    uint32_t fb_id;
    int fd[4];
    int fence_fd;

    uint32_t src_x, src_y, src_w, src_h;
    int crtc_x, crtc_y;
    uint32_t crtc_w, crtc_h;
    uint32_t zpos;

    struct drm_display *disp;
    int commit_to_video;
};

struct drm_buf_metadata {
    uint32_t fourcc;
    uint32_t width;
    uint32_t height;
    uint32_t flags;
};

struct drm_buf_import {
    uint32_t fourcc;
    uint32_t width;
    uint32_t height;
    uint32_t flags;
    int fd[4];
};

struct drm_display {
    int drm_fd;
    uint32_t width, height;
    uint32_t vrefresh;
    struct meson_device *dev;

    size_t nbuf;
    struct drm_buf *bufs;

    void (*destroy_display)(struct drm_display *disp);
    int (*alloc_bufs)(struct drm_display *disp, int num, struct drm_buf_metadata *info);
    int (*free_bufs)(struct drm_display *disp);

    struct drm_buf *(*alloc_buf)(struct drm_display *disp, struct drm_buf_metadata *info);
    struct drm_buf *(*import_buf)(struct drm_display *disp, struct drm_buf_import *info);
    int (*free_buf)(struct drm_buf *buf);
    int (*post_buf)(struct drm_display *disp, struct drm_buf *buf);

    void (*display_done_callback)(void *priv);
    void *priv;

    void (*resolution_change_callback)(void *priv);
    void *reso_priv;
};

struct drm_display *drm_display_init(void);
void drm_destroy_display(struct drm_display *disp);
void drm_display_register_done_cb(struct drm_display *disp, void *func, void *priv);
void drm_display_register_res_cb(struct drm_display *disp, void *func, void *priv);

int drm_alloc_bufs(struct drm_display *disp, int num, struct drm_buf_metadata *info);
int drm_free_bufs(struct drm_display *disp);

struct drm_buf *drm_alloc_buf(struct drm_display *disp, struct drm_buf_metadata *info);
struct drm_buf *drm_import_buf(struct drm_display *disp, struct drm_buf_import *info);
int drm_free_buf(struct drm_buf *buf);
int drm_post_buf(struct drm_display *disp, struct drm_buf *buf);

#endif
