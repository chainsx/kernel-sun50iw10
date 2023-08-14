/*
* Copyright (C) 2019 Allwinnertech Co.Ltd
* Authors: zhengwanyu
*
* This program is free software; you can redistribute  it and/or modify it
* under  the terms of  the GNU General  Public License as published by the
* Free Software Foundation;  either version 2 of the  License, or (at your
* option) any later version.
*
*/

#ifndef _UAPI_SUNXI_DRM_H_
#define _UAPI_SUNXI_DRM_H_

#include <video/sunxi_display2.h>
#include <uapi/drm/drm.h>

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * User-desired buffer creation information structure.
 *
 * @size: user-desired memory allocation size.
 *	- this size value would be page-aligned internally.
 * @flags: user request for setting memory type or cache attributes.
 * @handle: returned a handle to created gem object.
 *	- this handle will be set by gem module of kernel side.
 */
struct drm_sunxi_gem_create {
	__u64 size;
	__u32 flags;
	__u32 handle;
};

struct sunxi_drm_crtc_enhance {
	unsigned int crtc_obj_id;
	unsigned int mode;
	bool enable;
};

struct sunxi_drm_crtc_smbl {
	unsigned int crtc_obj_id;
	struct disp_rect window;
	bool enable;
};

struct sunxi_drm_phyaddr {
	/* return the physical address */
	__u64 phyaddr;
	/* dmabuf file descriptor */
	__s32 fd;
};

#define SUNXI_SET_ENHANCE	0x01
#define SUNXI_GET_ENHANCE	0x02
#define SUNXI_SET_SMBL	0x03
#define SUNXI_GET_SMBL	0x04
#define SUNXI_GEM_FD_TO_PHYADDR	0X05
#define SUNXI_GEM_CREATE	0X06

/* enhance */
#define DRM_IOCTL_SUNXI_SET_ENHANCE \
	DRM_IOWR(DRM_COMMAND_BASE + SUNXI_SET_ENHANCE, \
		struct sunxi_drm_crtc_enhance)

#define DRM_IOCTL_SUNXI_GET_ENHANCE \
	DRM_IOWR(DRM_COMMAND_BASE + SUNXI_GET_ENHANCE, \
		struct sunxi_drm_crtc_enhance)

/* smbl */
#define DRM_IOCTL_SUNXI_SET_SMBL \
	DRM_IOWR(DRM_COMMAND_BASE + SUNXI_SET_SMBL, \
		struct sunxi_drm_crtc_smbl)

#define DRM_IOCTL_SUNXI_GET_SMBL \
	DRM_IOWR(DRM_COMMAND_BASE + SUNXI_GET_SMBL, \
		struct sunxi_drm_crtc_smbl)

/* L2 */
#define DRM_IOCTL_SUNXI_GEM_FD_TO_PHYADDR \
	DRM_IOWR(DRM_COMMAND_BASE + SUNXI_GEM_FD_TO_PHYADDR, \
		struct sunxi_drm_phyaddr)

#define DRM_IOCTL_SUNXI_GEM_CREATE \
	DRM_IOWR(DRM_COMMAND_BASE + SUNXI_GEM_CREATE, \
		struct drm_sunxi_gem_create)

/* memory type definitions. */
enum e_drm_sunxi_gem_mem_type {
	/* Physically Continuous memory and used as default. */
	SUNXI_BO_CONTIG			= 0 << 0,
	/* Physically Non-Continuous memory. */
	SUNXI_BO_NONCONTIG		= 1 << 0,
	/* non-cachable mapping and used as default. */
	SUNXI_BO_NONCACHABLE	= 0 << 1,
	/* cachable mapping. */
	SUNXI_BO_CACHABLE		= 1 << 1,
	/* write-combine mapping. */
	SUNXI_BO_WC				= 1 << 2,
	SUNXI_BO_MASK			= SUNXI_BO_NONCONTIG | SUNXI_BO_CACHABLE |
						SUNXI_BO_WC
};

#if defined(__cplusplus)
}
#endif

#endif
