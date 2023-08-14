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
#include <drm/drm_crtc_helper.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_fourcc.h>

#include "sunxi_drm_bootlogo.h"
#include "sunxi_drm_gem.h"
#include "sunxi_drm_fb.h"
#include "sunxi_drm_fbdev.h"

#include <linux/memblock.h>

#ifndef MODULE
static u32 disp_reserve_size;
static u32 disp_reserve_base;
int disp_reserve_mem(bool reserve)
{
	if (!disp_reserve_size || !disp_reserve_base)
		return -EINVAL;

	if (reserve)
		memblock_reserve(disp_reserve_base, disp_reserve_size);
	else
		memblock_free(disp_reserve_base, disp_reserve_size);

	return 0;
}

static int __init early_disp_reserve(char *str)
{
	u32 temp[3] = {0};

	if (!str)
		return -EINVAL;

	get_options(str, 3, temp);

	disp_reserve_size = temp[1];
	disp_reserve_base = temp[2];
	if (temp[0] != 2 || disp_reserve_size <= 0)
		return -EINVAL;

	/* arch_mem_end = memblock_end_of_DRAM(); */
	disp_reserve_mem(true);
	pr_info("disp reserve base 0x%x ,size 0x%x\n",
					disp_reserve_base, disp_reserve_size);
	return 0;
}
early_param("disp_reserve", early_disp_reserve);
#endif

static void *sunxi_drm_map_kernel(unsigned long phys_addr,
				unsigned long size)
{
	int npages = PAGE_ALIGN(size) / PAGE_SIZE;
	struct page **pages = vmalloc(sizeof(struct page *) * npages);
	struct page **tmp = pages;
	struct page *cur_page = phys_to_page(phys_addr);
	pgprot_t pgprot;
	void *vaddr = NULL;
	int i;

	if (!pages)
		return NULL;

	for (i = 0; i < npages; i++)
		*(tmp++) = cur_page++;

	pgprot = pgprot_noncached(PAGE_KERNEL);
	vaddr = vmap(pages, npages, VM_MAP, pgprot);

	vfree(pages);
	return vaddr;
}

static void sunxi_drm_unmap_kernel(void *vaddr)
{
	vunmap(vaddr);
}

static int Fb_copy_boot_fb(char *fb_vaddr, struct bootlogo_info *bootlogo,
	unsigned int fb_w, unsigned int fb_h, unsigned int fb_bpp, unsigned int fb_stride)
{
	enum {
		BOOT_FB_ADDR = 0,
		BOOT_FB_WIDTH,
		BOOT_FB_HEIGHT,
		BOOT_FB_BPP,
		BOOT_FB_STRIDE,
		BOOT_FB_CROP_L,
		BOOT_FB_CROP_T,
		BOOT_FB_CROP_R,
		BOOT_FB_CROP_B,
	};

	unsigned int src_phy_addr = 0;
	char *src_addr = NULL;
	char *src_addr_b = NULL;
	char *src_addr_e = NULL;
	int src_width = 0;
	int src_height = 0;
	int src_bpp = 0;
	int src_stride = 0;
	int src_cp_btyes = 0;
	int src_crop_l = 0;
	int src_crop_t = 0;
	int src_crop_r = 0;
	int src_crop_b = 0;

	char *dst_addr = NULL;
	int dst_width = 0;
	int dst_height = 0;
	int dst_bpp = 0;
	int dst_stride = 0;

	unsigned long map_offset;

	src_phy_addr = bootlogo->phy_addr;
	src_width = bootlogo->width;
	src_height = bootlogo->height;
	src_bpp = bootlogo->bpp;
	src_stride = bootlogo->stride;
	src_crop_l = bootlogo->left;
	src_crop_t = bootlogo->top;
	src_crop_r = bootlogo->right;
	src_crop_b = bootlogo->bottom;

	dst_addr = fb_vaddr;
	dst_width = fb_w;
	dst_height = fb_h;
	dst_bpp = fb_bpp;
	dst_stride = fb_stride;

	if ((src_phy_addr == 0)
	    || (src_width <= 0)
	    || (src_height <= 0)
	    || (src_stride <= 0)
	    || (src_bpp <= 0)
	    || (dst_addr == NULL)
	    || (dst_width <= 0)
	    || (dst_height <= 0)
	    || (dst_stride <= 0)
	    || (dst_bpp <= 0)
	    || (src_bpp != dst_bpp)) {
		DRM_ERROR
		    ("wrong para: src[phy_addr=%d,w=%d,h=%d,bpp=%d,stride=%d], dst[addr=%p,w=%d,h=%d,bpp=%d,stride=%d]\n",
		     src_phy_addr,
		     src_width, src_height, src_bpp, src_stride, dst_addr,
		     dst_width, dst_height, dst_bpp, dst_stride);
		return -1;
	}

	map_offset = (unsigned long)src_phy_addr + PAGE_SIZE
	    - PAGE_ALIGN((unsigned long)src_phy_addr + 1);
	src_addr = (char *)sunxi_drm_map_kernel((unsigned long)src_phy_addr -
					       map_offset,
					       src_stride * src_height +
					       map_offset);
	if (src_addr == NULL) {
		DRM_ERROR("Fb_map_kernel_cache for src_addr failed\n");
		return -1;
	}

	src_addr_b = src_addr + map_offset;
	if ((src_crop_b > src_crop_t) &&
	    (src_height > src_crop_b - src_crop_t) &&
	    (src_crop_t >= 0) &&
	    (src_height >= src_crop_b)) {
		src_height = src_crop_b - src_crop_t;
		src_addr_b += (src_stride * src_crop_t);
	}
	if ((src_crop_r > src_crop_l)
	    && (src_width > src_crop_r - src_crop_l)
	    && (src_crop_l >= 0)
	    && (src_width >= src_crop_r)) {
		src_width = src_crop_r - src_crop_l;
		src_addr_b += (src_crop_l * src_bpp >> 3);
	}
	if (src_height < dst_height) {
		int dst_crop_t = (dst_height - src_height) >> 1;

		dst_addr += (dst_stride * dst_crop_t);
	} else if (src_height > dst_height) {
		DRM_ERROR("src_height(%d) > dst_height(%d),please cut the height\n",
		      src_height,
		      dst_height);
		sunxi_drm_unmap_kernel(src_addr);
		return -1;
	}
	if (src_width < dst_width) {
		int dst_crop_l = (dst_width - src_width) >> 1;

		dst_addr += (dst_crop_l * dst_bpp >> 3);
	} else if (src_width > dst_width) {
		DRM_ERROR("src_width(%d) > dst_width(%d),please cut the width!\n",
		      src_width,
		      dst_width);
		sunxi_drm_unmap_kernel(src_addr);
		return -1;
	}

	src_cp_btyes = src_width * src_bpp >> 3;
	src_addr_e = src_addr_b + src_stride * src_height;
	for (; src_addr_b != src_addr_e; src_addr_b += src_stride) {
		memcpy((void *)dst_addr, (void *)src_addr_b, src_cp_btyes);
		dst_addr += dst_stride;
	}

	sunxi_drm_unmap_kernel(src_addr);

#ifndef MODULE
		disp_reserve_mem(false);
#endif

	return 0;
}

void sunxi_drm_get_bootlogoinfo_from_dts_info(
	struct bootlogo_info *bootlogo, char *dts_attr)
{
	char *end = dts_attr;
	unsigned long long value;

	bootlogo->pics_format = BOOTLOGO_FORMAT_OTHERS;

	value = simple_strtoull(end, &end, 16);
	bootlogo->phy_addr = (unsigned int)value;
	if ((*end != ' ') && (*end != ',')) {
			 DRM_ERROR("error separator:%c\n", *end);
			 return;
	}

	value = simple_strtoull(end + 1, &end, 16);
	bootlogo->width = (unsigned int)value;
	if ((*end != ' ') && (*end != ',')) {
			 DRM_ERROR("error separator:%c\n", *end);
			 return;
	}

	value = simple_strtoull(end + 1, &end, 16);
	bootlogo->height = (unsigned int)value;
	if ((*end != ' ') && (*end != ',')) {
			 DRM_ERROR("error separator:%c\n", *end);
			 return;
	}

	value = simple_strtoull(end + 1, &end, 16);
	bootlogo->bpp = (unsigned int)value;
	if ((*end != ' ') && (*end != ',')) {
			 DRM_ERROR("error separator:%c\n", *end);
			 return;
	}

	value = simple_strtoull(end + 1, &end, 16);
	bootlogo->stride = (unsigned int)value;
	if ((*end != ' ') && (*end != ',')) {
			 DRM_ERROR("error separator:%c\n", *end);
			 return;
	}

	value = simple_strtoull(end + 1, &end, 16);
	bootlogo->left = (unsigned int)value;
	if ((*end != ' ') && (*end != ',')) {
			 DRM_ERROR("error separator:%c\n", *end);
			 return;
	}

	value = simple_strtoull(end + 1, &end, 16);
	bootlogo->top = (unsigned int)value;
	if ((*end != ' ') && (*end != ',')) {
			 DRM_ERROR("error separator:%c\n", *end);
			 return;
	}

	value = simple_strtoull(end + 1, &end, 16);
	bootlogo->right = (unsigned int)value;
	if ((*end != ' ') && (*end != ',')) {
			 DRM_ERROR("error separator:%c\n", *end);
			 return;
	}

	value = simple_strtoull(end + 1, &end, 16);
	bootlogo->bottom = (unsigned int)value;

	DRM_INFO("phyaddr:0x%x w-h:%ux%u bpp:%u stride:%u  crop:(%u, %u)-(%u, %u)\n",
		bootlogo->phy_addr, bootlogo->width, bootlogo->height,
		bootlogo->bpp, bootlogo->stride,
		bootlogo->left, bootlogo->top, bootlogo->right, bootlogo->bottom);

	return;
}

void sunxi_drm_get_bootlogoinfo_from_bmp_header(
	struct bootlogo_info *bootlogo, struct bmp_header *header,
	unsigned int fb_base)
{
	bootlogo->pics_format = BOOTLOGO_FORMAT_BMP;

	bootlogo->phy_addr = fb_base + header->data_offset;
	bootlogo->width = header->width;
	bootlogo->height = header->height;
	bootlogo->bpp = header->bit_count;
	bootlogo->stride = header->width * (header->bit_count / 8);
	bootlogo->left = 0;
	bootlogo->top = 0;
	bootlogo->right = header->width;
	bootlogo->bottom = (header->height & 0x80000000) ?
			(-header->height) : (header->height);
}

/* get the header of bmp and parse it to check if it is good */
int sunxi_drm_parse_bmp_header(struct bmp_header *header,
					unsigned long phy_addr)
{
	unsigned int bytes_per_pixel;
	struct bmp_header *tmp_header;

	tmp_header = sunxi_drm_map_kernel(phy_addr, sizeof(*tmp_header));
	if (!tmp_header) {
		DRM_ERROR("map bmp header failed\n");
		return -1;
	}

	if ((tmp_header->signature[0] != 'B') || (tmp_header->signature[1] != 'M')) {
		DRM_ERROR("This is NOT a bmp picture, signature:%c %c\n",
			tmp_header->signature[0], tmp_header->signature[1]);
		return -1;
	}

	bytes_per_pixel = tmp_header->bit_count / 8;
	if ((bytes_per_pixel != 3) && (bytes_per_pixel != 4)) {
		DRM_ERROR("error bmp bit counts per pixel:%d\n", tmp_header->bit_count);
		return -1;
	}
	DRM_INFO("bmp file, w:%d h:%d\n", tmp_header->width, tmp_header->height);

	memcpy(header, tmp_header, sizeof(*tmp_header));

	sunxi_drm_unmap_kernel(tmp_header);
	return 0;
}

static int sunxi_drm_copy_bootlogo(unsigned char *d_addr, unsigned int w,
			unsigned int h, struct bootlogo_info *bootlogo)
{
	unsigned char *src_addr;
	unsigned int i;
	unsigned int x, y;
	unsigned int effective_width, effective_height;
	void *screen_offset = NULL;
	unsigned int bytes_per_pixel;

	if (bootlogo->height & 0x80000000)
		src_addr = sunxi_drm_map_kernel(bootlogo->phy_addr,
			(-bootlogo->height) * bootlogo->stride);
	else
		src_addr = sunxi_drm_map_kernel(bootlogo->phy_addr,
			bootlogo->height * bootlogo->stride);

	if (!src_addr) {
		DRM_ERROR("sunxi_drm_map_kernel failed\n");
		return -1;
	}

	x = bootlogo->width;
	y = (bootlogo->height & 0x80000000) ?
			(-bootlogo->height) : (bootlogo->height);

	effective_width = (w < x) ? w : x;
	effective_height = (h < y) ? h : y;

	bytes_per_pixel = bootlogo->bpp >> 3;

	DRM_INFO("x-y:%dx%d  w-h:%dx%d  effective:%dx%d  bytes_per_pixel:%d\n",
		x, y, w, h, effective_width, effective_height, bytes_per_pixel);

	src_addr += PAGE_SIZE;

	/* if the param 'height' in bmp file is negative, it means that
	  * the data is in normal sequence, or it is inverse sequence that the first line
	  * normal is the last line in bmp file(mirror)
	  */
	if (((bootlogo->pics_format == BOOTLOGO_FORMAT_BMP) && (bootlogo->height & 0x80000000))
		|| (bootlogo->pics_format == BOOTLOGO_FORMAT_OTHERS)) {
		DRM_INFO("%s  Start Copy bootlogo data:\n", __func__);
		screen_offset =
			(void *)((void *__force)d_addr
				+ (w * (abs(h - y) / 2) + abs(w - x) / 2)
				* bytes_per_pixel);

		for (i = 0; i < effective_height; i++) {
			memcpy((void *)screen_offset, src_addr, effective_width * bytes_per_pixel);
			screen_offset = (void *)(screen_offset + w * bytes_per_pixel);
			src_addr = (void *)src_addr + x * bytes_per_pixel;
		}
	} else {
		screen_offset =
			(void *)((void *__force)d_addr +
				 (w * (abs(h - y) / 2) + abs(w - x) / 2)
				 * bytes_per_pixel);
		src_addr =
			(void *)(src_addr + (x * (abs(y - h) / 2)
			+ abs(x - w) / 2) * bytes_per_pixel);

		src_addr =
			(void *)src_addr + (effective_height - 1) * x * bytes_per_pixel;
		for (i = effective_height - 1; i >= 0; i--) {
			memcpy((void *)screen_offset, src_addr, effective_width * bytes_per_pixel);
			screen_offset =
				(void *)(screen_offset + w * bytes_per_pixel);
			src_addr =
				(void *)src_addr + i * x * bytes_per_pixel;
		}
	}

	sunxi_drm_unmap_kernel(src_addr);

	return 0;
}


int sunxi_drm_fbdev_copy_bootlogo(
	struct drm_device *dev, struct bootlogo_info *bootlogo)
{
	unsigned char *fbdev_addr;

	unsigned int x, y, i;
	unsigned int fbdev_w, fbdev_h, fbdev_bpp, fbdev_pitches;

	struct sunxi_drm_fb *sunxi_fb;
	struct sunxi_drm_fbdev *fbdev;

	/* copy the bootlogo to every fbdev fb that has been created */
	for (i = 0; i < FBDEV_MAX_NUM; i++) {
			fbdev = sunxi_drm_fbdev_get_fbdev(i);
			sunxi_fb = fbdev->sunxi_fb;
			if (!sunxi_fb) {
				DRM_ERROR("fbdev do NOT have a coresponsible fb\n");
				return -1;
			}

			if (!sunxi_fb->obj[0]) {
				DRM_ERROR("fbdev's fb do NOT have a coresponsible gem\n");
				return -1;
			}

			fbdev_addr = sunxi_fb->obj[0]->vaddr + sunxi_fb->fb.offsets[0];
			fbdev_w = sunxi_fb->fb.width;
			fbdev_h = sunxi_fb->fb.height / 2;
			fbdev_bpp = sunxi_fb->fb.format->cpp[0] * 8;
			fbdev_pitches = sunxi_fb->fb.pitches[0];

			x = bootlogo->width;
			y = (bootlogo->height & 0x80000000) ?
				(-bootlogo->height) : (bootlogo->height);

			DRM_INFO("fbdev_addr:%px  offset:%d  "
			"fbdev_w:%d fbdev_h:%d fbdev_bpp:%d pitch:%d  x:%d y:%d\n",
				sunxi_fb->obj[0]->vaddr, sunxi_fb->fb.offsets[0],
				fbdev_w, fbdev_h, fbdev_bpp, fbdev_pitches, x, y);

			/* if bootlogo is bigger than the fb of fbdev,
			it is NOT allowed to copy bootlogo to fbdev */
			if ((x > fbdev_w) || (y > fbdev_h)) {
				DRM_ERROR("The weight and height of bmp "
				"is NOT suit for fbdev, bmp:%d-%d fbdev:%d-%d\n",
				x, y, fbdev_w, fbdev_h);
				return -1;
			}

			if (fbdev_bpp != bootlogo->bpp) {
				DRM_ERROR("bmp's bpp is NOT suit for fbdev: %d %d\n",
					fbdev_bpp, bootlogo->bpp);
				return -1;
			}

			/* sunxi_drm_copy_bootlogo(fbdev_addr, fbdev_w,
						fbdev_h, bootlogo); */
			Fb_copy_boot_fb(fbdev_addr, bootlogo,
				fbdev_w, fbdev_h, fbdev_bpp, fbdev_pitches);
	}
	return 0;
}

