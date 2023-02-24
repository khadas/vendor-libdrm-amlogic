 /*
  * include/uapi/drm/meson_drm.h
  *
  * Copyright (C) 2016 Amlogic, Inc. All rights reserved.
  *
  * This program is free software; you can redistribute  it and/or modify it
  * under  the terms of  the GNU General  Public License as published by the
  * Free Software Foundation;  either version 2 of the License, or (at your
  * option) any later version.
  */

#ifndef _MESON_DRM_H
#define _MESON_DRM_H

#include <drm/drm.h>

/* Use flags */
#define MESON_USE_NONE			0
#define MESON_USE_SCANOUT			(1ull << 0)
#define MESON_USE_CURSOR			(1ull << 1)
#define MESON_USE_RENDERING		(1ull << 2)
#define MESON_USE_LINEAR			(1ull << 3)
#define MESON_USE_PROTECTED		(1ull << 11)
#define MESON_USE_HW_VIDEO_ENCODER		(1ull << 12)
#define MESON_USE_CAMERA_WRITE		(1ull << 13)
#define MESON_USE_CAMERA_READ		(1ull << 14)
#define MESON_USE_TEXTURE			(1ull << 17)
#define MESON_USE_VIDEO_PLANE		(1ull << 18)
#define MESON_USE_VIDEO_AFBC		(1ull << 19)
#define MESON_USE_VD1			(1ull << 20)
#define MESON_USE_VD2			(1ull << 21)

/**
 * User-desired buffer creation information structure.
 *
 * @size: user-desired memory allocation size.
 * @flags: user request for setting memory type or cache attributes.
 * @handle: returned a handle to created gem object.
 *     - this handle will be set by gem module of kernel side.
 */
struct drm_meson_gem_create {
	__u64 size;
	__u32 flags;
	__u32 handle;
};

/*Memory related.*/
#define DRM_IOCTL_MESON_GEM_CREATE		DRM_IOWR(DRM_COMMAND_BASE + \
		0x00, struct drm_meson_gem_create)

/*KMS related.*/
#define DRM_IOCTL_MESON_ASYNC_ATOMIC	DRM_IOWR(DRM_COMMAND_BASE + \
		0x10, struct drm_mode_atomic)

#endif /* _MESON_DRM_H */
