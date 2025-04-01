/* sunxi_drm_gem.h
 *
 * Copyright (C) 2022 Allwinnertech Co., Ltd.
 * Authors: zhengwanyu <zhengwanyu@allwinnertech.com>
 * Authors: hongyaobin <hongyaobin@allwinnertech.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#ifndef _SUNXI_DRM_GEM_H_
#define _SUNXI_DRM_GEM_H_

#include <drm/drm_gem.h>

/**
 * struct sunxi_drm_gem_object - GEM object backed by sunxi memory allocations
 * @base: a gem object.
 *	- a new handle to this gem object would be created
 *	by drm_gem_handle_create().
 * @flags: indicate memory type to allocated buffer and cache attruibute.
 * @size: size requested from user, in bytes and this size is aligned
 *	in page unit.
 * @cookie: cookie returned by dma_alloc_attrs
 * @dma_addr: bus address(accessed by dma) to allocated memory region.
 *	- this address could be physical address without IOMMU and
 *	device address with IOMMU.
 * @dma_attrs: use this attribute to set the DMA mapping API.
 * 	- this is defined at the "include/linux/dma-mapping.h".
 * @pages: Array of backing pages.
 * @sgt: Imported sg_table.
 * @vaddr: kernel virtual address of the backing memory
 *
 */
struct sunxi_drm_gem_object {
	struct drm_gem_object base;
	size_t flags;
	size_t size;
	dma_addr_t dma_addr;
	unsigned long dma_attrs;
	struct page	**pages;
	struct sg_table *sgt;

	/* use to store the result of dma_alloc_attrs */
	void *vaddr;
};

extern const struct vm_operations_struct sunxi_drm_gem_vm_ops;

static inline struct sunxi_drm_gem_object *to_sunxi_drm_gem_obj(
					struct drm_gem_object *gem_obj)
{
	return container_of(gem_obj, struct sunxi_drm_gem_object, base);
}

void sunxi_gem_dump(struct drm_gem_object *gem_obj);
ssize_t sunxi_drm_gem_show(char *buf,
					struct sunxi_drm_gem_object *sunxi_gem_obj,
					unsigned int offset, unsigned int pitches);

/* destroy a buffer with gem object */
void sunxi_drm_gem_destroy(struct sunxi_drm_gem_object *sunxi_gem_obj);

/* free GEM object */
void sunxi_drm_gem_free_object(struct drm_gem_object *gem_obj);

/* create memory region for DRM framebuffer */
int sunxi_drm_gem_dumb_create(struct drm_file *file_priv,
			    struct drm_device *drm,
			    struct drm_mode_create_dumb *args);

/* map memory region for DRM framebuffer to user space */
int sunxi_drm_gem_dumb_map_offset(struct drm_file *file_priv,
				struct drm_device *drm, u32 handle,
				u64 *offset);

/* set vm_flags and we can change the VM attribute to other one at here */
int sunxi_drm_gem_mmap(struct file *filp, struct vm_area_struct *vma);

/* allocate physical memory */
struct sunxi_drm_gem_object *sunxi_drm_gem_create(struct drm_device *drm,
							size_t flags,
							size_t size);

/*
 * request gem object creation and buffer allocation as the size
 * that it is calculated with framebuffer information such as width,
 * height and bpp.
 */
int sunxi_drm_gem_create_ioctl(struct drm_device *dev, void *data,
			   struct drm_file *file_priv);

/* prime */
struct sg_table *sunxi_drm_gem_prime_get_sg_table(struct drm_gem_object *obj);
struct drm_gem_object *
sunxi_drm_gem_prime_import_sg_table(struct drm_device *dev,
				  struct dma_buf_attachment *attach,
				  struct sg_table *sgt);
int sunxi_drm_gem_prime_mmap(struct drm_gem_object *obj,
			   struct vm_area_struct *vma);
void *sunxi_drm_gem_prime_vmap(struct drm_gem_object *obj);
void sunxi_drm_gem_prime_vunmap(struct drm_gem_object *obj, void *vaddr);
int sunxi_drm_gem_get_phyaddr_ioctl(struct drm_device *dev,
						void *data, struct drm_file *file);
#endif
