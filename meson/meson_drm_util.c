/*
 * Copyright (C) 2020 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include "meson_drm_util.h"

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
