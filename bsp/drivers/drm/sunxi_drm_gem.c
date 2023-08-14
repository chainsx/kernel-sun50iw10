/* sunxi_drm_gem.c
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

#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/export.h>
#include <linux/dma-buf.h>
#include <linux/dma-mapping.h>

#include <drm/drm.h>
#include <drm/drm_vma_manager.h>
#include <drm/drm_prime.h>
#include "sunxi_drm_gem.h"
#include "sunxi_drm_drv.h"
#include "sunxi_drm_iommu.h"
#include "drm/sunxi_drm.h"

static int sunxi_drm_alloc_buf(struct sunxi_drm_gem_object *sunxi_gem_obj)
{
	struct drm_device *dev = sunxi_gem_obj->base.dev;
	unsigned long attr = 0;
	unsigned int nr_pages;
	struct sg_table sgt;
	int ret = -ENOMEM;

	if (sunxi_gem_obj->dma_addr) {
		DRM_DEBUG_KMS("already allocated.\n");
		return 0;
	}

	sunxi_gem_obj->dma_attrs = 0;

	/*
	 * if SUNXI_BO_CONTIG, fully physically contiguous memory
	 * region will be allocated else physically contiguous
	 * as possible.
	 */
	if (!(sunxi_gem_obj->flags & SUNXI_BO_NONCONTIG))
		sunxi_gem_obj->dma_attrs |= DMA_ATTR_FORCE_CONTIGUOUS;

	/*
	 * if SUNXI_BO_WC or SUNXI_BO_NONCACHABLE, writecombine mapping
	 * else cachable mapping.
	 */
	if (sunxi_gem_obj->flags & SUNXI_BO_WC ||
			!(sunxi_gem_obj->flags & SUNXI_BO_CACHABLE))
		attr = DMA_ATTR_WRITE_COMBINE;

	sunxi_gem_obj->dma_attrs |= attr;
	sunxi_gem_obj->dma_attrs |= DMA_ATTR_NO_KERNEL_MAPPING;

	nr_pages = sunxi_gem_obj->size >> PAGE_SHIFT;

	sunxi_gem_obj->pages = kvmalloc_array(nr_pages, sizeof(struct page *),
			GFP_KERNEL | __GFP_ZERO);
	if (!sunxi_gem_obj->pages) {
		DRM_ERROR("failed to allocate pages.\n");
		return -ENOMEM;
	}

	sunxi_gem_obj->vaddr = dma_alloc_attrs(dev->dev, sunxi_gem_obj->size,
					     &sunxi_gem_obj->dma_addr, GFP_KERNEL,
					     sunxi_gem_obj->dma_attrs);
	if (!sunxi_gem_obj->vaddr) {
		DRM_ERROR("failed to allocate buffer.\n");
		goto err_free;
	}

	ret = dma_get_sgtable_attrs(dev->dev, &sgt, sunxi_gem_obj->vaddr,
				    sunxi_gem_obj->dma_addr, sunxi_gem_obj->size,
				    sunxi_gem_obj->dma_attrs);
	if (ret < 0) {
		DRM_ERROR("failed to get sgtable.\n");
		goto err_dma_free;
	}

	if (drm_prime_sg_to_page_addr_arrays(&sgt, sunxi_gem_obj->pages, NULL,
					     nr_pages)) {
		DRM_ERROR("invalid sgtable.\n");
		ret = -EINVAL;
		goto err_sgt_free;
	}

	sg_free_table(&sgt);

	DRM_DEBUG_KMS("dma_addr(0x%lx), size(0x%lx)\n",
			(unsigned long)sunxi_gem_obj->dma_addr, sunxi_gem_obj->size);

	return 0;

	err_sgt_free:
		sg_free_table(&sgt);
	err_dma_free:
		dma_free_attrs(dev->dev, sunxi_gem_obj->size, sunxi_gem_obj->vaddr,
				   sunxi_gem_obj->dma_addr, sunxi_gem_obj->dma_attrs);
	err_free:
		kvfree(sunxi_gem_obj->pages);

		return ret;
}

static void sunxi_drm_free_buf(struct sunxi_drm_gem_object *sunxi_gem_obj)
{
	struct drm_device *dev = sunxi_gem_obj->base.dev;

	if (!sunxi_gem_obj->dma_addr) {
		DRM_DEBUG_KMS("dma_addr is invalid.\n");
		return;
	}

	DRM_DEBUG_KMS("dma_addr(0x%lx), size(0x%lx)\n",
			(unsigned long)sunxi_gem_obj->dma_addr, sunxi_gem_obj->size);

	dma_free_attrs(dev->dev, sunxi_gem_obj->size, sunxi_gem_obj->vaddr,
			(dma_addr_t)sunxi_gem_obj->dma_addr,
			sunxi_gem_obj->dma_attrs);

	kvfree(sunxi_gem_obj->pages);
}

void sunxi_gem_dump(struct drm_gem_object *gem_obj)
{
	struct sunxi_drm_gem_object *sunxi_gem_obj = to_sunxi_drm_gem_obj(gem_obj);

	DRM_INFO("gem_obj: size:%u name:%d handle count:%u\n",
			(unsigned int)gem_obj->size, gem_obj->name,
			gem_obj->handle_count);
	DRM_INFO("phy_addr:0x%lx virt_addr:0x%lx sgt:%p\n",
				(unsigned long)sunxi_gem_obj->dma_addr,
				(unsigned long)sunxi_gem_obj->vaddr,
						sunxi_gem_obj->sgt);
}

ssize_t sunxi_drm_gem_show(char *buf,
					struct sunxi_drm_gem_object *sunxi_gem_obj,
					unsigned int offset, unsigned int pitches)
{
	ssize_t n = 0;
	struct drm_gem_object *gem_obj = &sunxi_gem_obj->base;

	n += sprintf(buf + n, "gem_obj: size:%u name:%d handle count:%u\n",
			(unsigned int)gem_obj->size, gem_obj->name,
			gem_obj->handle_count);
	n += sprintf(buf + n, "phy_addr:0x%lx virt_addr:0x%lx sgt:%p\n",
				(unsigned long)sunxi_gem_obj->dma_addr + offset,
				(unsigned long)sunxi_gem_obj->vaddr + offset,
						sunxi_gem_obj->sgt);
	return n;
}

int sunxi_drm_gem_get_phyaddr_ioctl(struct drm_device *dev,
						void *data, struct drm_file *file)
{
	struct sunxi_drm_phyaddr *arg = data;
	struct drm_gem_object *gem_obj = NULL;
	struct sg_table *sgt = NULL;
	struct page *page = NULL;
	int ret;
	uint32_t handle = 0;
	ret = drm_gem_prime_fd_to_handle(dev, file, arg->fd, &handle);
	if (ret) {
		DRM_ERROR("fd %d to handle failed", arg->fd);
		return -1;
	}
	gem_obj = drm_gem_object_lookup(file, handle);
	if (gem_obj == NULL) {
		DRM_ERROR("gem object not finde fd %d, handle 0x%x", arg->fd, handle);
		return -1;
	}
	sgt = sunxi_drm_gem_prime_get_sg_table(gem_obj);
	if (sgt == NULL) {
		DRM_ERROR("gem prime get sg_table failed");
		drm_gem_object_put(gem_obj);
		return -1;
	}
	page = sg_page(sgt->sgl);
	arg->phyaddr = PFN_PHYS(page_to_pfn(page));
	drm_gem_object_put(gem_obj);
	return 0;
}

void sunxi_drm_gem_destroy(struct sunxi_drm_gem_object *sunxi_gem_obj)
{
	struct drm_gem_object *obj = &sunxi_gem_obj->base;

	DRM_DEBUG_KMS("handle count = %d\n", obj->handle_count);

	/*
	 * do not release memory region from exporter.
	 *
	 * the region will be released by exporter
	 * once dmabuf's refcount becomes 0.
	 */
	if (obj->import_attach)
		drm_prime_gem_destroy(obj, sunxi_gem_obj->sgt);
	else
		sunxi_drm_free_buf(sunxi_gem_obj);

	/* release file pointer to gem object. */
	drm_gem_object_release(obj);

	kfree(sunxi_gem_obj);
}

/**
 * sunxi_drm_gem_obj_init - Create a sunxi GEM object
 *			WITHOUT allocating memory
 * @dev: DRM device
 * @size: size of the object to allocate
 *
 * This function creates and initializes a sunxi GEM object of the given size,
 * but doesn't allocate any memory to back the object.
 *
 * Returns:
 * A struct sunxi_drm_gem_object * on success or an ERR_PTR()-encoded negative
 * error code on failure.
 */
static struct sunxi_drm_gem_object *
sunxi_drm_gem_obj_init(struct drm_device *dev, size_t size)
{
	struct sunxi_drm_gem_object *sunxi_gem_obj;
	struct drm_gem_object *gem_obj;
	int ret;

	sunxi_gem_obj = kzalloc(sizeof(*sunxi_gem_obj), GFP_KERNEL);
	if (!sunxi_gem_obj)
		return ERR_PTR(-ENOMEM);

	sunxi_gem_obj->size = size;
	gem_obj = &sunxi_gem_obj->base;

	ret = drm_gem_object_init(dev, gem_obj, size);
	if (ret) {
		DRM_ERROR("failed to initialize gem object\n");
		goto error;
	}
	ret = drm_gem_create_mmap_offset(gem_obj);
	if (ret) {
		drm_gem_object_release(gem_obj);
		goto error;
	}

	return sunxi_gem_obj;

error:
	kfree(sunxi_gem_obj);
	return ERR_PTR(ret);
}

/**
 * sunxi_drm_gem_create - allocate an object with the given size
 * @drm: DRM device
 * @size: size of the object to allocate
 *
 * This function creates a SUNXI GEM object and allocates a contiguous chunk of
 * memory as backing store. The backing memory has the writecombine attribute
 * set.
 *
 * Returns:
 * A struct sunxi_drm_gem_object * on success or an ERR_PTR()-encoded negative
 * error code on failure.
 */
struct sunxi_drm_gem_object *
sunxi_drm_gem_create(struct drm_device *dev,
							size_t flags,
							size_t size)
{
	struct sunxi_drm_gem_object *sunxi_gem_obj;
	int ret;

	if (flags & ~(SUNXI_BO_MASK)) {
		DRM_ERROR("invalid GEM buffer flags: %lu\n", flags);
		return ERR_PTR(-EINVAL);
	}

	if (!size) {
		DRM_ERROR("invalid GEM buffer size: %lu\n", size);
		return ERR_PTR(-EINVAL);
	}

	size = round_up(size, PAGE_SIZE);

	sunxi_gem_obj = sunxi_drm_gem_obj_init(dev, size);
	if (IS_ERR(sunxi_gem_obj))
		return sunxi_gem_obj;

	if (!is_drm_iommu_supported(dev) && (flags & SUNXI_BO_NONCONTIG)) {
		/*
		 * when no IOMMU is available, all allocated buffers are
		 * contiguous anyway, so drop SUNXI_BO_NONCONTIG flag
		 */
		flags &= ~SUNXI_BO_NONCONTIG;
		DRM_WARN("Non-contiguous allocation is not supported without IOMMU, falling back to contiguous buffer\n");
	}

	/* set memory type and cache attribute from user side. */
	sunxi_gem_obj->flags = flags;

	ret = sunxi_drm_alloc_buf(sunxi_gem_obj);
	if (ret < 0) {
		drm_gem_object_release(&sunxi_gem_obj->base);
		kfree(sunxi_gem_obj);
		return ERR_PTR(ret);
	}

	return sunxi_gem_obj;
}

static int sunxi_drm_gem_handle_create(struct drm_gem_object *obj,
			       struct drm_file *file_priv,
			       unsigned int *handle)
{
	int ret;

	/*
	 * allocate a id of idr table where the obj is registered
	 * and handle has the id what user can see.
	 */
	ret = drm_gem_handle_create(file_priv, obj, handle);
	if (ret)
		return ret;

	/* drop reference from allocate - handle holds it now. */
	drm_gem_object_put(obj);

	return 0;
}

int sunxi_drm_gem_create_ioctl(struct drm_device *dev, void *data,
			   struct drm_file *file_priv)
{
	struct drm_sunxi_gem_create *args = data;
	struct sunxi_drm_gem_object *sunxi_gem_obj;
	int ret;

	sunxi_gem_obj = sunxi_drm_gem_create(dev, args->flags, args->size);
	if (IS_ERR(sunxi_gem_obj))
		return PTR_ERR(sunxi_gem_obj);

	ret = sunxi_drm_gem_handle_create(&sunxi_gem_obj->base, file_priv,
						&args->handle);
	if (ret) {
		sunxi_drm_gem_destroy(sunxi_gem_obj);
		return ret;
	}

	return 0;
}

/**
 * sunxi_drm_gem_free_object - free resources associated with a SUNXI GEM object
 * @gem_obj: GEM object to free
 *
 * This function frees the backing memory of the SUNXI GEM object, cleans up the
 * GEM object state and frees the memory used to store the object itself.
 */
void sunxi_drm_gem_free_object(struct drm_gem_object *gem_obj)
{
	sunxi_drm_gem_destroy(to_sunxi_drm_gem_obj(gem_obj));
}

/**
 * sunxi_drm_gem_dumb_create - create a dumb buffer object
 * @file_priv: DRM file-private structure to create the dumb buffer for
 * @drm: DRM device
 * @args: IOCTL data
 *
 * This function computes the pitch of the dumb buffer and rounds it up to an
 * integer number of bytes per pixel. Drivers for hardware that doesn't have
 * any additional restrictions on the pitch can directly use this function as
 * their ->dumb_create() callback.
 *
 * NOTE!!!!!!:
 * DRM_IOCTL_MODE_CREATE_DUMB--->(dev->driver->)dumb_create()
 *	--->sunxi_drm_gem_dumb_create()
 *
 * For hardware with additional restrictions, drivers can adjust the fields
 * set up by userspace and pass the IOCTL data along to the
 * sunxi_drm_gem_dumb_create_internal() function.
 *
 * Returns:
 * 0 on success or a negative error code on failure.
 */
int sunxi_drm_gem_dumb_create(struct drm_file *file_priv,
			    struct drm_device *dev,
			    struct drm_mode_create_dumb *args)
{
	struct sunxi_drm_gem_object *sunxi_gem_obj;
	unsigned int flags;
	int ret;

	args->pitch = DIV_ROUND_UP(args->width * args->bpp, 8);
	args->size = args->pitch * args->height;

	if (is_drm_iommu_supported(dev))
		flags = SUNXI_BO_NONCONTIG | SUNXI_BO_WC;
	else
		flags = SUNXI_BO_CONTIG | SUNXI_BO_WC;

	sunxi_gem_obj = sunxi_drm_gem_create(dev, flags, args->size);
	if (IS_ERR(sunxi_gem_obj))
		return PTR_ERR(sunxi_gem_obj);

	ret = sunxi_drm_gem_handle_create(&sunxi_gem_obj->base, file_priv,
						 &args->handle);
	if (ret) {
		sunxi_drm_gem_destroy(sunxi_gem_obj);
		return ret;
	}

	return 0;
}

/**
 * sunxi_drm_gem_dumb_map_offset - return the fake mmap offset for a SUNXI GEM
 *     object
 * @file_priv: DRM file-private structure containing the GEM object
 * @drm: DRM device
 * @handle: GEM object handle
 * @offset: return location for the fake mmap offset
 *
 * This function look up an object by its handle and returns the fake mmap
 * offset associated with it. Drivers using the SUNXI function should set this
 * as their DRM driver's ->dumb_map_offset() callback.
 *
 * NOTE!!!!!!:
 * DRM_IOCTL_MODE_MAP_DUMB--->(dev->driver->)dumb_map_offset
 *	--->sunxi_drm_gem_dumb_map_offset()
 *
 * Returns:
 * 0 on success or a negative error code on failure.
 */
int sunxi_drm_gem_dumb_map_offset(struct drm_file *file_priv,
				struct drm_device *drm, u32 handle,
				u64 *offset)
{
	struct drm_gem_object *gem_obj;

	gem_obj = drm_gem_object_lookup(file_priv, handle);
	if (!gem_obj) {
		dev_err(drm->dev, "failed to lookup GEM object\n");
		return -EINVAL;
	}

	*offset = drm_vma_node_offset_addr(&gem_obj->vma_node);

	drm_gem_object_put(gem_obj);

	return 0;
}

const struct vm_operations_struct sunxi_drm_gem_vm_ops = {
	.open = drm_gem_vm_open,
	.close = drm_gem_vm_close,
};

static int sunxi_drm_gem_mmap_buffer(struct sunxi_drm_gem_object *sunxi_gem_obj,
				      struct vm_area_struct *vma)
{
	struct drm_device *drm_dev = sunxi_gem_obj->base.dev;
	unsigned long vm_size;
	int ret;

	vma->vm_flags &= ~VM_PFNMAP;
	vma->vm_pgoff = 0;
	vm_size = vma->vm_end - vma->vm_start;

	/* check if user-requested size is valid. */
	if (vm_size > sunxi_gem_obj->size)
		return -EINVAL;

	ret = dma_mmap_attrs(drm_dev->dev, vma, sunxi_gem_obj->vaddr,
			     sunxi_gem_obj->dma_addr, sunxi_gem_obj->size,
			     sunxi_gem_obj->dma_attrs);
	if (ret < 0) {
		DRM_ERROR("failed to mmap.\n");
		return ret;
	}

	return 0;
}

static int sunxi_drm_gem_mmap_obj(struct drm_gem_object *obj,
				struct vm_area_struct *vma)
{
	int ret;
	struct sunxi_drm_gem_object *sunxi_gem_obj = to_sunxi_drm_gem_obj(obj);

	/* non-cachable as default. */
	if (sunxi_gem_obj->flags & SUNXI_BO_CACHABLE)
		vma->vm_page_prot = vm_get_page_prot(vma->vm_flags);
	else if (sunxi_gem_obj->flags & SUNXI_BO_WC)
		vma->vm_page_prot =
			pgprot_writecombine(vm_get_page_prot(vma->vm_flags));
	else
		vma->vm_page_prot =
			pgprot_noncached(vm_get_page_prot(vma->vm_flags));

	ret = sunxi_drm_gem_mmap_buffer(sunxi_gem_obj, vma);
	if (ret)
		goto err_close_vm;

	return ret;

	err_close_vm:
		drm_gem_vm_close(vma);

		return ret;
}

/**
 * sunxi_drm_gem_mmap - memory-map a SUNXI GEM object, For fops->mmap()
 * @filp: file object
 * @vma: VMA for the area to be mapped
 *
 * This function implements an augmented version of the GEM DRM file mmap
 * operation for SUNXI objects: In addition to the usual GEM VMA setup it
 * immediately faults in the entire object instead of using on-demaind
 * faulting.
 *
 * NOTE!!!!!!:
 * (user-space)mmap--->sunxi_drm_gem_mmap()
 *
 * Returns:
 * 0 on success or a negative error code on failure.
 */
int sunxi_drm_gem_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct drm_gem_object *gem_obj;
	int ret;

	ret = drm_gem_mmap(filp, vma);
	if (ret < 0) {
		DRM_ERROR("failed to mmap.\n");
		return ret;
	}

	gem_obj = vma->vm_private_data;

	if (gem_obj->import_attach)
		return dma_buf_mmap(gem_obj->dma_buf, vma, 0);

	return sunxi_drm_gem_mmap_obj(gem_obj, vma);
}

/**
 * sunxi_drm_gem_prime_get_sg_table - provide a scatter/gather table of pinned
 *     pages for a SUNXI GEM object
 * @obj: GEM object
 *
 * This function exports a scatter/gather table suitable for PRIME usage by
 * calling the standard DMA mapping API. Drivers should set this as their DRM
 * driver's ->gem_prime_get_sg_table() callback.
 *
 * Returns:
 * A pointer to the scatter/gather table of pinned pages or NULL on failure.
 */
struct sg_table *sunxi_drm_gem_prime_get_sg_table(struct drm_gem_object *obj)
{
	struct sunxi_drm_gem_object *sunxi_gem_obj = to_sunxi_drm_gem_obj(obj);
	int npages;
	struct drm_device *drm_dev = obj->dev;

	npages = sunxi_gem_obj->size >> PAGE_SHIFT;

	return drm_prime_pages_to_sg(drm_dev, sunxi_gem_obj->pages, npages);
}

/**
 * sunxi_drm_gem_prime_import_sg_table - produce a SUNXI GEM object from another
 *     driver's scatter/gather table of pinned pages
 * @dev: device to import into
 * @attach: DMA-BUF attachment
 * @sgt: scatter/gather table of pinned pages
 *
 * This function imports a scatter/gather table exported via DMA-BUF by
 * another driver. Imported buffers must be physically contiguous in memory
 * (i.e. the scatter/gather table must contain a single entry). Drivers should
 * set this as their DRM driver's ->gem_prime_import_sg_table() callback.
 *
 * Returns:
 * A pointer to a newly created GEM object or an ERR_PTR-encoded negative
 * error code on failure.
 */
struct drm_gem_object *
sunxi_drm_gem_prime_import_sg_table(struct drm_device *dev,
				  struct dma_buf_attachment *attach,
				  struct sg_table *sgt)
{
	struct sunxi_drm_gem_object *sunxi_gem_obj;
	int npages;
	int ret;

	sunxi_gem_obj = sunxi_drm_gem_obj_init(dev, attach->dmabuf->size);
	if (IS_ERR(sunxi_gem_obj))  {
		ret = PTR_ERR(sunxi_gem_obj);
		return ERR_PTR(ret);
	}

	sunxi_gem_obj->dma_addr = sg_dma_address(sgt->sgl);

	npages = sunxi_gem_obj->size >> PAGE_SHIFT;
	sunxi_gem_obj->pages = kvmalloc_array(npages, sizeof(struct page *), GFP_KERNEL);
	if (!sunxi_gem_obj->pages) {
		ret = -ENOMEM;
		goto err;
	}

	ret = drm_prime_sg_to_page_addr_arrays(sgt, sunxi_gem_obj->pages, NULL,
								npages);
	if (ret < 0)
		goto err_free_large;

	sunxi_gem_obj->sgt = sgt;

	if (sgt->nents == 1) {
		/* always physically continuous memory if sgt->nents is 1. */
		sunxi_gem_obj->flags |= SUNXI_BO_CONTIG;
	} else {
		/*
		 * this case could be CONTIG or NONCONTIG type but for now
		 * sets NONCONTIG.
		 */
		sunxi_gem_obj->flags |= SUNXI_BO_NONCONTIG;
	}

	return &sunxi_gem_obj->base;

err_free_large:
	kvfree(sunxi_gem_obj->pages);
err:
	drm_gem_object_release(&sunxi_gem_obj->base);
	kfree(sunxi_gem_obj);
	return ERR_PTR(ret);
}

/**
 * sunxi_drm_gem_prime_mmap - memory-map an exported SUNXI GEM object
 * @obj: GEM object
 * @vma: VMA for the area to be mapped
 *
 * This function maps a buffer imported via DRM PRIME into a userspace
 * process's address space. Drivers should set this as their DRM
 * driver's ->gem_prime_mmap() callback.
 *
 * Returns:
 * 0 on success or a negative error code on failure.
 */
int sunxi_drm_gem_prime_mmap(struct drm_gem_object *obj,
			   struct vm_area_struct *vma)
{
	int ret;

	ret = drm_gem_mmap_obj(obj, obj->size, vma);
	if (ret < 0)
		return ret;

	return sunxi_drm_gem_mmap_obj(obj, vma);
}

/**
 * sunxi_drm_gem_prime_vmap - map a SUNXI GEM object into the kernel's virtual
 *     address space
 * @obj: GEM object
 *
 * This function maps a buffer exported via DRM PRIME into the kernel's
 * virtual address space. Since the SUNXI buffers are already mapped into the
 * kernel virtual address space this simply returns the cached virtual
 * address. Drivers should set this as their
 * DRM driver's ->gem_prime_vmap() callback.
 *
 * Returns:
 * The kernel virtual address of the SUNXI GEM object's backing store.
 */
void *sunxi_drm_gem_prime_vmap(struct drm_gem_object *obj)
{
	struct sunxi_drm_gem_object *sunxi_obj = to_sunxi_drm_gem_obj(obj);

	return sunxi_obj->vaddr;
}

/**
 * sunxi_drm_gem_prime_vunmap - unmap a SUNXI GEM object from the kernel's
 * virtual address space
 * @obj: GEM object
 * @vaddr: kernel virtual address where the SUNXI GEM object was mapped
 *
 * This function removes a buffer exported via DRM PRIME from the kernel's
 * virtual address space. This is a no-op because SUNXI buffers cannot be
 * unmapped from kernel space. Drivers should set this as their
 * DRM driver's ->gem_prime_vunmap() callback.
 */
void sunxi_drm_gem_prime_vunmap(struct drm_gem_object *obj, void *vaddr)
{
	/* Nothing to do */
}

