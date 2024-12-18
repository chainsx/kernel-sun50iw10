/*
 * drivers/video/fbdev/sunxi/disp2/disp/de/disp_rotation_sw/disp_rotation_sw.c
 *
 * Copyright (c) 2007-2018 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifdef CONFIG_AW_DISP2_NEON_ROTATION
#include <asm/neon.h>
#endif

#include "disp_rotation_sw.h"

#define ROTATION_SW_ALIGN 4

#define DOUBLE_BUF

#define MB_SIZE 32
#define MB_SIZE_U8 MB_SIZE
#define MB_SIZE_U16 (MB_SIZE >> 1)
#define MB_SIZE_U32 (MB_SIZE >> 2)

#define MY_BYTE_ALIGN(x) (((x + (4*1024-1)) >> 12) << 12)

struct buf_info {
	unsigned long long   phy_addr;
	unsigned int         vir_addr;
	unsigned int         size;
	struct disp_rect            update_rect;
};

struct rotation_buf_info {
	struct buf_info *w_buf;
	struct buf_info *r_buf;
	struct buf_info buf_a;
	struct buf_info buf_b;
	struct buf_info buf_c;
};

typedef struct disp_rotation_sw_private_data {
	int enable;
	int degree;
	int bytes_pp;
	struct rotation_buf_info tg_buf;
	struct disp_layer_config     lyr_bak;
#if defined(__LINUX_PLAT__)
	spinlock_t data_lock;
#else
	int data_lock;
#endif
} prv_data_t;

#if defined(__LINUX_PLAT__)
	static spinlock_t s_data_lock;
#else
	static int s_data_lock;
#endif

struct disp_rotation_sw *g_rot_sw;
prv_data_t *rot_sw_p;

static void Fb_unmap_kernel(void *vaddr) { vunmap(vaddr); }

#if 0
extern int flush_user_range(long start, long end);
#else
static int flush_user_range(long start, long end)
{
	return 0;
}
#endif

void *disp_malloc_cache(u32 num_bytes, u32 *phys_addr)
{
	return disp_malloc(num_bytes, phys_addr);
}

static void *map_kernel_cache(unsigned long phys_addr, unsigned long size)
{
	int npages = PAGE_ALIGN(size) / PAGE_SIZE;
	struct page **pages = vmalloc(sizeof(struct page *) * npages);
	struct page **tmp = pages;
	struct page *cur_page = phys_to_page(phys_addr);
	pgprot_t pgprot;
	void *vaddr;
	int i;

	if (!pages)
		return 0;

	for (i = 0; i < npages; i++)
		*(tmp++) = cur_page++;

	pgprot = pgprot_noncached(PAGE_KERNEL);
	vaddr = vmap(pages, npages, VM_MAP, pgprot);

	vfree(pages);
	return vaddr;
}

extern void rot_90_u32(
	unsigned int *src_addr, const unsigned int src_widthstep,
	const unsigned int src_width, const unsigned int src_height,
	unsigned int *dst_addr, const unsigned int dst_widthstep);

struct disp_rotation_sw *disp_get_rotation_sw(u32 disp)
{
	u32 num_screens;

	num_screens = bsp_disp_feat_get_num_screens();
	if (disp >= num_screens || NULL == g_rot_sw) {
		DE_WRN("disp %d out of range? g_rot_sw=%d\n",
			disp, (int)g_rot_sw);
		return NULL;
	}
	return &g_rot_sw[disp];
}

static void rot_270_u8_buf(
	unsigned char *src_addr, const unsigned int src_widthstep,
	const unsigned int src_width, const unsigned int src_height,
	unsigned char *dst_addr, const unsigned int dst_widthstep)
{
	unsigned char buf[MB_SIZE_U8][MB_SIZE_U8];

	rot_270_u8(src_addr, src_widthstep, src_width,
		src_height, (unsigned char *)buf, MB_SIZE);
	rot_0_u8((unsigned char *)buf, MB_SIZE, src_height,
		src_width, dst_addr, dst_widthstep);
}

static void rot_270_u16_buf(
	unsigned short *src_addr, const unsigned int src_widthstep,
	const unsigned int src_width, const unsigned int src_height,
	unsigned short *dst_addr, const unsigned int dst_widthstep)
{
	unsigned short buf[MB_SIZE_U16][MB_SIZE_U16];

	rot_270_u16(src_addr, src_widthstep, src_width,
		src_height, (unsigned short *)buf, MB_SIZE);
	rot_0_u16((unsigned short *)buf, MB_SIZE, src_height,
		src_width, dst_addr, dst_widthstep);
}

static void rot_270_u32_buf(
	unsigned int *src_addr, const unsigned int src_widthstep,
	const unsigned int src_width, const unsigned int src_height,
	unsigned int *dst_addr, const unsigned int dst_widthstep)
{
	unsigned int buf[MB_SIZE_U32][MB_SIZE_U32];

	rot_270_u32(src_addr, src_widthstep, src_width,
		src_height, (unsigned int *)buf, MB_SIZE);
	rot_0_u32((unsigned int *)buf, MB_SIZE, src_height,
		src_width, dst_addr, dst_widthstep);
}

static void rot_90_u8_buf(
	unsigned char *src_addr, const unsigned int src_widthstep,
	const unsigned int src_width, const unsigned int src_height,
	unsigned char *dst_addr, const unsigned int dst_widthstep)
{
	unsigned char buf[MB_SIZE_U8][MB_SIZE_U8];

	rot_90_u8(src_addr, src_widthstep, src_width,
		src_height, (unsigned char *)buf, MB_SIZE);
	rot_0_u8((unsigned char *)buf, MB_SIZE, src_height,
		src_width, dst_addr, dst_widthstep);
}

static void rot_90_u16_buf(
	unsigned short *src_addr, const unsigned int src_widthstep,
	const unsigned int src_width, const unsigned int src_height,
	unsigned short *dst_addr, const unsigned int dst_widthstep)
{
	unsigned short buf[MB_SIZE_U16][MB_SIZE_U16];

	rot_90_u16(src_addr, src_widthstep, src_width,
		src_height, (unsigned short *)buf, MB_SIZE);
	rot_0_u16((unsigned short *)buf, MB_SIZE, src_height,
		src_width, dst_addr, dst_widthstep);
}

static void rot_90_u32_buf(
	unsigned int *src_addr, const unsigned int src_widthstep,
	const unsigned int src_width, const unsigned int src_height,
	unsigned int *dst_addr, const unsigned int dst_widthstep)
{
	unsigned int buf[MB_SIZE_U32][MB_SIZE_U32];

#ifdef CONFIG_AW_DISP2_NEON_ROTATION
kernel_neon_begin();
	rot_90_u32(src_addr, src_widthstep, src_width,
		src_height, (unsigned int *)buf, MB_SIZE);
kernel_neon_end();
#else
	rot_90_u32(src_addr, src_widthstep, src_width,
		src_height, (unsigned int *)buf, MB_SIZE);
#endif

	rot_0_u32((unsigned int *)buf, MB_SIZE, src_height,
		src_width, dst_addr, dst_widthstep);
}

static int rot_270_u8_mb(
	unsigned char *src_addr, const unsigned int src_widthstep,
	const unsigned int src_width, const unsigned int src_height,
	unsigned char *dst_addr, const unsigned int dst_widthstep)

{
	unsigned int delta_w;
	unsigned int delta_h;
	unsigned int src_widthstep_buf;
	unsigned char *src_addr_end, *p_src;

	unsigned int dst_widthstep_buf;
	unsigned char *p_dst, *p_dst_end;
	const char *dst_vir_addr = (const char *)dst_addr;

	delta_w = src_width % MB_SIZE_U8;
	delta_h = src_height % MB_SIZE_U8;

	src_widthstep_buf = MB_SIZE_U8 * src_widthstep;
	dst_widthstep_buf = MB_SIZE_U8 * dst_widthstep;

	src_addr = src_addr + src_width - MB_SIZE_U8;
	src_addr_end = src_addr - src_width + delta_w;

	for (; src_addr != src_addr_end; src_addr -= MB_SIZE_U8) {
		p_src = src_addr;
		p_dst = dst_addr;
		p_dst_end = p_dst + src_height - delta_h;
		for (; p_dst != p_dst_end; p_dst += MB_SIZE_U8) {
			rot_270_u8_buf(p_src, src_widthstep, MB_SIZE_U8,
				MB_SIZE_U8, p_dst, dst_widthstep);
			p_src += src_widthstep_buf;
		}
		dst_addr += dst_widthstep_buf;
	}

	src_addr += (MB_SIZE_U8 - delta_w);
	p_dst_end = dst_addr + src_height - delta_h;
	for (; dst_addr != p_dst_end; dst_addr += MB_SIZE_U8) {
		rot_270_u8_buf(src_addr, src_widthstep, delta_w,
			MB_SIZE_U8, dst_addr, dst_widthstep);
		src_addr += src_widthstep_buf;
	}

	rot_270_u8_buf(src_addr, src_widthstep, delta_w,
		delta_h, dst_addr, dst_widthstep);

	src_addr = src_addr + src_width - MB_SIZE_U8;
	dst_addr -= (dst_widthstep * (src_width - delta_w));
	src_addr_end = src_addr - src_width + delta_w;
	for (; src_addr != src_addr_end; src_addr -= MB_SIZE_U8) {
		rot_270_u8_buf(src_addr, src_widthstep, MB_SIZE_U8,
			delta_h, dst_addr, dst_widthstep);
		dst_addr += dst_widthstep_buf;
	}

	flush_user_range((long)dst_vir_addr, (long)(dst_vir_addr
		 + dst_widthstep * (src_width - 1) + src_height));

	return 0;
}

static int rot_270_u16_mb(
	unsigned short *src_addr, const unsigned int src_widthstep,
	const unsigned int src_width, const unsigned int src_height,
	unsigned short *dst_addr, const unsigned int dst_widthstep)
{
	unsigned int delta_w;
	unsigned int delta_h;

	unsigned int src_widthstep_buf;
	unsigned short *src_addr_end, *p_src;

	unsigned int dst_widthstep_buf;
	unsigned short *p_dst, *p_dst_end;
	const char *dst_vir_addr = (const char *)dst_addr;

	delta_w = src_width % MB_SIZE_U16;
	delta_h = src_height % MB_SIZE_U16;

	src_widthstep_buf = MB_SIZE_U16 * src_widthstep;
	dst_widthstep_buf = MB_SIZE_U16 * dst_widthstep;

	src_addr = src_addr + src_width - MB_SIZE_U16;
	src_addr_end = src_addr - src_width + delta_w;
	for (; src_addr != src_addr_end; src_addr -= MB_SIZE_U16) {
		p_src = src_addr;
		p_dst = dst_addr;
		p_dst_end = p_dst + src_height - delta_h;
		for (; p_dst != p_dst_end; p_dst += MB_SIZE_U16) {
			rot_270_u16_buf(p_src, src_widthstep, MB_SIZE_U16,
				MB_SIZE_U16, p_dst, dst_widthstep);
			p_src = (unsigned short *)((char *)p_src + src_widthstep_buf);
		}
		dst_addr = (unsigned short *)((char *)dst_addr + dst_widthstep_buf);
	}

	src_addr += (MB_SIZE_U16 - delta_w);
	p_dst_end = dst_addr + src_height - delta_h;
	for (; dst_addr != p_dst_end; dst_addr += MB_SIZE_U16) {
		rot_270_u16_buf(src_addr, src_widthstep, delta_w,
			MB_SIZE_U16, dst_addr, dst_widthstep);
		src_addr = (unsigned short *)((char *)src_addr + src_widthstep_buf);
	}

	rot_270_u16_buf(src_addr, src_widthstep, delta_w,
		delta_h, dst_addr, dst_widthstep);

	src_addr = src_addr + src_width - MB_SIZE_U16;
	dst_addr = (unsigned short *)((char *)dst_addr
		 - dst_widthstep * (src_width - delta_w));
	src_addr_end = src_addr - src_width + delta_w;
	for (; src_addr != src_addr_end; src_addr -= MB_SIZE_U16) {
		rot_270_u16_buf(src_addr, src_widthstep, MB_SIZE_U16,
			delta_h, dst_addr, dst_widthstep);
		dst_addr = (unsigned short *)((char *)dst_addr + dst_widthstep_buf);
	}

	flush_user_range((long)dst_vir_addr, (long)(dst_vir_addr
		 + dst_widthstep * (src_width - 1) + (src_height << 1)));

	return 0;
}

static int rot_270_u32_mb(
	unsigned int *src_addr, const unsigned int src_widthstep,
	const unsigned int src_width, const unsigned int src_height,
	unsigned int *dst_addr, const unsigned int dst_widthstep)

{
	unsigned int delta_w;
	unsigned int delta_h;

	unsigned int src_widthstep_buf;
	unsigned int *src_addr_end, *p_src;

	unsigned int dst_widthstep_buf;
	unsigned int *p_dst, *p_dst_end;
	const char *dst_vir_addr = (const char *)dst_addr;

	delta_w = src_width % MB_SIZE_U32;
	delta_h = src_height % MB_SIZE_U32;

	src_widthstep_buf = MB_SIZE_U32 * src_widthstep;
	dst_widthstep_buf = MB_SIZE_U32 * dst_widthstep;

	src_addr = src_addr + src_width - MB_SIZE_U32;
	src_addr_end = src_addr - src_width + delta_w;
	for (; src_addr != src_addr_end; src_addr -= MB_SIZE_U32) {
		p_src = src_addr;
		p_dst = dst_addr;
		p_dst_end = p_dst + src_height - delta_h;
		for (; p_dst != p_dst_end; p_dst += MB_SIZE_U32) {
			rot_270_u32_buf(p_src, src_widthstep, MB_SIZE_U32,
				MB_SIZE_U32, p_dst, dst_widthstep);
			p_src = (unsigned int *)((char *)p_src + src_widthstep_buf);
		}
		dst_addr = (unsigned int *)((char *)dst_addr + dst_widthstep_buf);
	}

	src_addr += (MB_SIZE_U32 - delta_w);
	p_dst_end = dst_addr + src_height - delta_h;
	for (; dst_addr != p_dst_end; dst_addr += MB_SIZE_U32) {
		rot_270_u32_buf(src_addr, src_widthstep, delta_w,
			MB_SIZE_U32, dst_addr, dst_widthstep);
		src_addr = (unsigned int *)((char *)src_addr + src_widthstep_buf);
	}

	rot_270_u32_buf(src_addr, src_widthstep, delta_w,
		delta_h, dst_addr, dst_widthstep);

	src_addr = src_addr + src_width - MB_SIZE_U32;
	dst_addr = (unsigned int *)((char *)dst_addr
		 - dst_widthstep * (src_width - delta_w));

	src_addr_end = src_addr - src_width + delta_w;
	for (; src_addr != src_addr_end; src_addr -= MB_SIZE_U32) {
		rot_270_u32_buf(src_addr, src_widthstep, MB_SIZE_U32,
			delta_h, dst_addr, dst_widthstep);
		dst_addr = (unsigned int *)((char *)dst_addr + dst_widthstep_buf);
	}

	flush_user_range((long)dst_vir_addr, (long)(dst_vir_addr
		 + dst_widthstep * (src_width - 1) + (src_height << 2)));

	return 0;
}

static int rot_90_u8_mb(
	unsigned char *src_addr, const unsigned int src_widthstep,
	const unsigned int src_width, const unsigned int src_height,
	unsigned char *dst_addr, const unsigned int dst_widthstep)
{
	unsigned int delta_w;
	unsigned int delta_h;

	unsigned int src_widthstep_buf;
	unsigned char *src_addr_end, *p_src;

	unsigned int dst_widthstep_buf;
	unsigned char *p_dst, *p_dst_end;
	const char *dst_vir_addr = (const char *)dst_addr;

	delta_w = src_width % MB_SIZE_U8;
	delta_h = src_height % MB_SIZE_U8;

	src_widthstep_buf = MB_SIZE_U8 * src_widthstep;
	dst_widthstep_buf = MB_SIZE_U8 * dst_widthstep;

	src_addr += (src_widthstep * ((signed int)src_height - delta_h - MB_SIZE_U8));
	src_addr_end = src_addr + src_width - delta_w;
	dst_addr += delta_h;
	for (; src_addr != src_addr_end; src_addr += MB_SIZE_U8) {
		p_src = src_addr;
		p_dst = dst_addr;
		p_dst_end = p_dst + src_height - delta_h;
		for (; p_dst != p_dst_end; p_dst += MB_SIZE_U8) {
			rot_90_u8_buf(p_src, src_widthstep, MB_SIZE_U8,
				MB_SIZE_U8, p_dst, dst_widthstep);
			p_src -= src_widthstep_buf;
		}
		dst_addr += dst_widthstep_buf;
	}

	p_dst_end = dst_addr + src_height - delta_h;
	for (; dst_addr != p_dst_end; dst_addr += MB_SIZE_U8) {
		rot_90_u8_buf(src_addr, src_widthstep, delta_w,
			MB_SIZE_U8, dst_addr, dst_widthstep);
		src_addr -= src_widthstep_buf;
	}

	src_addr = src_addr - src_width + delta_w
		 + src_widthstep * (src_height + MB_SIZE_U8 - delta_h);
	dst_addr = (unsigned char *)dst_vir_addr;
	src_addr_end = src_addr + src_width - delta_w;
	for (; src_addr != src_addr_end; src_addr += MB_SIZE_U8) {
		rot_90_u8_buf(src_addr, src_widthstep, MB_SIZE_U8,
			delta_h, dst_addr, dst_widthstep);
		dst_addr += dst_widthstep_buf;
	}

	rot_90_u8_buf(src_addr, src_widthstep, delta_w,
		delta_h, dst_addr, dst_widthstep);

	flush_user_range((long)dst_vir_addr, (long)(dst_vir_addr
		 + dst_widthstep * (src_width - 1) + src_height));

	return 0;
}

static int rot_90_u16_mb(
	unsigned short *src_addr, const unsigned int src_widthstep,
	const unsigned int src_width, const unsigned int src_height,
	unsigned short *dst_addr, const unsigned int dst_widthstep)
{
	unsigned int delta_w;
	unsigned int delta_h;

	unsigned int src_widthstep_buf;
	unsigned short *src_addr_end, *p_src;

	unsigned int dst_widthstep_buf;
	unsigned short *p_dst, *p_dst_end;
	const char *dst_vir_addr = (const char *)dst_addr;

	delta_w = src_width % MB_SIZE_U16;
	delta_h = src_height % MB_SIZE_U16;

	src_widthstep_buf = MB_SIZE_U16 * src_widthstep;
	dst_widthstep_buf = MB_SIZE_U16 * dst_widthstep;

	src_addr = (unsigned short *)((char *)src_addr + src_widthstep
		 * ((signed int)src_height - delta_h - MB_SIZE_U16));
	src_addr_end = src_addr + src_width - delta_w;
	dst_addr += delta_h;
	for (; src_addr != src_addr_end; src_addr += MB_SIZE_U16) {
		p_src = src_addr;
		p_dst = dst_addr;
		p_dst_end = p_dst + src_height - delta_h;
		for (; p_dst != p_dst_end; p_dst += MB_SIZE_U16) {
			rot_90_u16_buf(p_src, src_widthstep, MB_SIZE_U16,
				MB_SIZE_U16, p_dst, dst_widthstep);
			p_src = (unsigned short *)((char *)p_src -
							src_widthstep_buf);
		}
		dst_addr = (unsigned short *)((char *)dst_addr +
							dst_widthstep_buf);
	}

	p_dst_end = dst_addr + src_height - delta_h;
	for (; dst_addr != p_dst_end; dst_addr += MB_SIZE_U16) {
		rot_90_u16_buf(src_addr, src_widthstep, delta_w,
			MB_SIZE_U16, dst_addr, dst_widthstep);
		src_addr = (unsigned short *)((char *)src_addr -
							src_widthstep_buf);
	}

	src_addr = (unsigned short *)((char *)src_addr
		 + src_widthstep * (src_height + MB_SIZE_U16 - delta_h)
		 - ((src_width - delta_w) << 1));
	dst_addr = (unsigned short *)dst_vir_addr;
	src_addr_end = src_addr + src_width - delta_w;
	for (; src_addr != src_addr_end; src_addr += MB_SIZE_U16) {
		rot_90_u16_buf(src_addr, src_widthstep, MB_SIZE_U16,
			delta_h, dst_addr, dst_widthstep);
		dst_addr = (unsigned short *)((char *)dst_addr +
							dst_widthstep_buf);
	}

	rot_90_u16_buf(src_addr, src_widthstep, delta_w,
		delta_h, dst_addr, dst_widthstep);

	flush_user_range((long)dst_vir_addr, (long)(dst_vir_addr
		 + dst_widthstep * (src_width - 1) + (src_height << 1)));

	return 0;
}

static int rot_90_u32_mb(
	unsigned int *src_addr, const unsigned int src_widthstep,
	const unsigned int src_width, const unsigned int src_height,
	unsigned int *dst_addr, const unsigned int dst_widthstep)
{
	unsigned int delta_w;
	unsigned int delta_h;

	unsigned int src_widthstep_buf;
	unsigned int *src_addr_end, *p_src;

	unsigned int dst_widthstep_buf;
	unsigned int *p_dst, *p_dst_end;
	const char *dst_vir_addr = (const char *)dst_addr;

	delta_w = src_width % MB_SIZE_U32;
	delta_h = src_height % MB_SIZE_U32;

	src_widthstep_buf = MB_SIZE_U32 * src_widthstep;
	dst_widthstep_buf = MB_SIZE_U32 * dst_widthstep;

	src_addr = (unsigned int *)((char *)src_addr
		 + src_widthstep * ((signed int)src_height - delta_h - MB_SIZE_U32));
	src_addr_end = src_addr + src_width - delta_w;
	dst_addr += delta_h;
	for (; src_addr != src_addr_end; src_addr += MB_SIZE_U32) {
		p_src = src_addr;
		p_dst = dst_addr;
		p_dst_end = p_dst + src_height - delta_h;
		for (; p_dst != p_dst_end; p_dst += MB_SIZE_U32) {
			rot_90_u32_buf(p_src, src_widthstep, MB_SIZE_U32,
				MB_SIZE_U32, p_dst, dst_widthstep);
			p_src = (unsigned int *)((char *)p_src - src_widthstep_buf);
		}
		dst_addr = (unsigned int *)((char *)dst_addr + dst_widthstep_buf);
	}

	p_dst_end = dst_addr + src_height - delta_h;
	for (; dst_addr != p_dst_end; dst_addr += MB_SIZE_U32) {
		rot_90_u32_buf(src_addr, src_widthstep, delta_w,
			MB_SIZE_U32, dst_addr, dst_widthstep);
		src_addr = (unsigned int *)((char *)src_addr - src_widthstep_buf);
	}

	src_addr = (unsigned int *)((char *)src_addr
		 + src_widthstep * (src_height + MB_SIZE_U32 - delta_h)
		 - ((src_width - delta_w) << 2));
	dst_addr = (unsigned int *)dst_vir_addr;
	src_addr_end = src_addr + src_width - delta_w;
	for (; src_addr != src_addr_end; src_addr += MB_SIZE_U32) {
		rot_90_u32_buf(src_addr, src_widthstep, MB_SIZE_U32,
			delta_h, dst_addr, dst_widthstep);
		dst_addr = (unsigned int *)((char *)dst_addr + dst_widthstep_buf);
	}

	rot_90_u32_buf(src_addr, src_widthstep, delta_w,
		delta_h, dst_addr, dst_widthstep);

	flush_user_range((long)dst_vir_addr, (long)(dst_vir_addr
		 + dst_widthstep * (src_width - 1) + (src_height << 2)));

	return 0;
}

static int rotate_fb(struct disp_fb_info *src_fb, struct disp_rect *src_dirty,
	struct disp_fb_info *dst_fb, struct disp_rect *dst_dirty, void *dst_vir_addr, int deg)
{
	unsigned long map_size;
	void *src_vir_addr;
	int src_widthstep, src_offset;
	int dst_widthstep, dst_offset;
	int Bpp[3];
	int dirty_fac1, dirty_fac2;

	switch (src_fb->format) {
	case DISP_FORMAT_ARGB_8888:
	case DISP_FORMAT_ABGR_8888:
	case DISP_FORMAT_RGBA_8888:
	case DISP_FORMAT_BGRA_8888:
		map_size = src_fb->size[0].width * src_fb->size[0].height << 2;
		Bpp[0] = 4;
		Bpp[1] = Bpp[2] = 0;
		break;
	case DISP_FORMAT_YUV420_SP_UVUV:
	case DISP_FORMAT_YUV420_SP_VUVU:
		map_size = src_fb->size[0].width * src_fb->size[0].height * 3 >> 1;
		Bpp[0] = 1;
		Bpp[1] = 2;
		Bpp[2] = 0;
		dirty_fac1 = 1;
		break;
	case DISP_FORMAT_YUV420_P:
		map_size = src_fb->size[0].width * src_fb->size[0].height * 3 >> 1;
		Bpp[0] = Bpp[1] = Bpp[2] = 1;
		dirty_fac1 = dirty_fac2 = 1;
	default:
		DE_WRN("fixme");
		return -1;
	}

	src_vir_addr = map_kernel_cache((unsigned long)src_fb->addr[0], map_size);
	if (NULL == src_vir_addr) {
		DE_WRN("map_kernel_cache failed!\n");
		return -1;
	}

	switch (Bpp[0]) {
	case 4:
		src_widthstep = src_fb->size[0].width << 2;
		src_offset = src_widthstep * src_dirty->y + (src_dirty->x << 2);
		dst_widthstep = dst_fb->size[0].width << 2;
		dst_offset = dst_widthstep * dst_dirty->y + (dst_dirty->x << 2);
		switch (deg) {
		case ROTATION_SW_90:

#ifdef CONFIG_AW_DISP2_NEON_ROTATION
		    kernel_neon_begin();
			rot_90_u32((unsigned int *)((char *)src_vir_addr + src_offset),
				src_widthstep, src_dirty->width, src_dirty->height,
				(unsigned int *)((char *)dst_vir_addr + dst_offset), dst_widthstep);
			kernel_neon_end();
#else
			rot_90_u32_mb((unsigned int *)((char *)src_vir_addr + src_offset),
				src_widthstep, src_dirty->width, src_dirty->height,
				(unsigned int *)((char *)dst_vir_addr + dst_offset), dst_widthstep);
#endif
			break;
		case ROTATION_SW_270:
			rot_270_u32_mb((unsigned int *)((char *)src_vir_addr + src_offset),
				src_widthstep, src_dirty->width, src_dirty->height,
				(unsigned int *)((char *)dst_vir_addr + dst_offset), dst_widthstep);
			break;
		default:
			DE_WRN("fixme: deg=%d", deg);
			goto go_out;
		}
		break;
	case 1:
		src_widthstep = src_fb->size[0].width;
		src_offset = src_widthstep * src_dirty->y + src_dirty->x;
		dst_widthstep = dst_fb->size[0].width;
		dst_offset = dst_widthstep * dst_dirty->y + dst_dirty->x;
		switch (deg) {
		case ROTATION_SW_90:
			rot_90_u8_mb((char *)src_vir_addr + src_offset,
				src_widthstep, src_dirty->width, src_dirty->height,
				(char *)dst_vir_addr + dst_offset, dst_widthstep);
			break;
		case ROTATION_SW_270:
			rot_270_u8_mb((char *)src_vir_addr + src_offset,
				src_widthstep, src_dirty->width, src_dirty->height,
				(char *)dst_vir_addr + dst_offset, dst_widthstep);
			break;
		default:
			DE_WRN("fixme: deg=%d", deg);
			goto go_out;
		}
		break;
	case 2:
	default:
		DE_WRN("fixme");
		goto go_out;
	}

	src_offset = src_widthstep * src_fb->size[0].height;
	dst_offset = dst_widthstep * dst_fb->size[0].height;

	switch (Bpp[1]) {
	case 2:
		src_widthstep = src_fb->size[0].width << 1;
		src_offset += src_widthstep * (src_dirty->y >> dirty_fac1)
			 + (src_dirty->x << 1 >> dirty_fac1);
		dst_widthstep = dst_fb->size[0].width << 1;
		dst_offset += dst_widthstep * (dst_dirty->y >> dirty_fac1)
			 + (dst_dirty->x << 1 >> dirty_fac1);
		switch (deg) {
		case ROTATION_SW_90:
			rot_90_u16_mb((unsigned short *)((char *)src_vir_addr + src_offset),
				src_widthstep, src_dirty->width >> dirty_fac1, src_dirty->height >> dirty_fac1,
				(unsigned short *)((char *)dst_vir_addr + dst_offset), dst_widthstep);
			break;
		case ROTATION_SW_270:
			rot_270_u16_mb((unsigned short *)((char *)src_vir_addr + src_offset),
				src_widthstep, src_dirty->width >> dirty_fac1, src_dirty->height >> dirty_fac1,
				(unsigned short *)((char *)dst_vir_addr + dst_offset), dst_widthstep);
			break;
		default:
			DE_WRN("fixme: deg=%d", deg);
			goto go_out;
		}
		break;
	case 0:
		break;
	case 4:
	case 1:
	default:
		DE_WRN("fixme: Bpp[1]=%d", Bpp[1]);
	}

go_out:
	Fb_unmap_kernel(src_vir_addr);
	return 0;
}

static int update_rotate_fb(struct buf_info *src, struct buf_info *dst,
	int Bpp, int widthstep, struct disp_rect *dst_dirty_rect, struct disp_rectsz *scn_win)
{

#if 0
	printk("update_rect[%d,%d,%d,%d], dst_dirty_rect[%d,%d,%d,%d],scn_win[%d,%d]\n",
		src->update_rect.x, src->update_rect.y, src->update_rect.width,
		src->update_rect.height, dst_dirty_rect->x, dst_dirty_rect->y,
		dst_dirty_rect->width, dst_dirty_rect->height, scn_win->width, scn_win->height);
#endif

#if defined(DOUBLE_BUF)
	if ((0 != src->vir_addr) && (0 != dst->vir_addr)
		&& ((dst_dirty_rect->width != scn_win->height)
		|| (dst_dirty_rect->height != scn_win->width))
		&& ((src->update_rect.x != dst_dirty_rect->x)
		|| (src->update_rect.y != dst_dirty_rect->y)
		|| (src->update_rect.width != dst_dirty_rect->width)
		|| (src->update_rect.height != dst_dirty_rect->height))) {
		int offset = widthstep * src->update_rect.y + Bpp * src->update_rect.x;
		void *src_addr = (void *)((char *)src->vir_addr + offset);
		void *dst_addr = (void *)((char *)dst->vir_addr + offset);
		rot_0_u32(src_addr, widthstep, src->update_rect.width,
			src->update_rect.height, dst_addr, widthstep);
	}
	memcpy((void *)&dst->update_rect, (void *)dst_dirty_rect, sizeof(struct disp_rect));
#endif
	return 0;
}

static int manager_rotate_buffer(struct disp_fb_info *ask_fb, struct rotation_buf_info *tg_buf)
{
	unsigned int ask_total_size = 0;
	unsigned int phy_addr = 0;
	unsigned int offset0 = 0;
	struct buf_info *buf = NULL;

	switch (ask_fb->format) {
	case DISP_FORMAT_ARGB_8888:
	case DISP_FORMAT_ABGR_8888:
	case DISP_FORMAT_RGBA_8888:
	case DISP_FORMAT_BGRA_8888:
		ask_total_size = (ask_fb->size[0].width * ask_fb->size[0].height) << 2;

#if defined(ONE_BUF)
		tg_buf->w_buf = &tg_buf->buf_a;
#elif defined(DOUBLE_BUF)
		if (tg_buf->w_buf == &tg_buf->buf_a) {
			tg_buf->w_buf = &tg_buf->buf_b;
			tg_buf->r_buf = &tg_buf->buf_a;
		} else {
			tg_buf->w_buf = &tg_buf->buf_a;
			tg_buf->r_buf = &tg_buf->buf_b;
		}
#elif defined(TRIPLE_BUF)
		if (tg_buf->w_buf == &tg_buf->buf_a) {
			tg_buf->w_buf = &tg_buf->buf_b;
			tg_buf->r_buf = &tg_buf->buf_a;
		} else if (tg_buf->w_buf == &tg_buf->buf_b) {
			tg_buf->w_buf = &tg_buf->buf_c;
			tg_buf->r_buf = &tg_buf->buf_b;
		} else {
			tg_buf->w_buf = &tg_buf->buf_a;
			tg_buf->r_buf = &tg_buf->buf_c;
		}
#endif
		break;
	case DISP_FORMAT_YUV420_SP_UVUV:
	case DISP_FORMAT_YUV420_SP_VUVU:
		ask_total_size = ask_fb->size[1].width * ask_fb->size[1].height * 6;
		offset0 = (unsigned int)(ask_fb->addr[1] - ask_fb->addr[0]);
		if (tg_buf->w_buf == &tg_buf->buf_b) {
			tg_buf->w_buf = &tg_buf->buf_a;
		} else {
			tg_buf->w_buf = &tg_buf->buf_b;
		}
		break;
	case DISP_FORMAT_YUV420_P:
	default:
		DE_WRN("not support this mode[%d]\n",
			ask_fb->format);
		return -1;
	}

	buf = tg_buf->w_buf;
	if (ask_total_size != buf->size) {
		if (0 != buf->size) {
			phy_addr = (unsigned int)buf->phy_addr;
			disp_free((void *)buf->vir_addr, (void *)phy_addr, buf->size);
			memset((void *)buf, 0, sizeof(struct buf_info));
		}
		buf->vir_addr = (unsigned int)disp_malloc_cache(ask_total_size, &phy_addr);
		if (0 == buf->vir_addr) {
			DE_WRN("malloc failed! size=%x\n", ask_total_size);
			return -1;
		}
		buf->phy_addr = (unsigned long long)phy_addr;
		buf->size = ask_total_size;
	}

	ask_fb->addr[0] = buf->phy_addr;
	switch (ask_fb->format) {
	case DISP_FORMAT_ARGB_8888:
	case DISP_FORMAT_ABGR_8888:
	case DISP_FORMAT_RGBA_8888:
	case DISP_FORMAT_BGRA_8888:
		break;
	case DISP_FORMAT_YUV420_SP_UVUV:
	case DISP_FORMAT_YUV420_SP_VUVU:
		ask_fb->addr[1] = ask_fb->addr[0] + offset0;
		break;
	case DISP_FORMAT_YUV420_P:
	default:
		DE_WRN("not support this mode[%d]\n",
			ask_fb->format);
		return -1;
	}

	return 0;
}

static int update_layer_info(int rot, struct disp_rectsz scn_size,
	struct disp_rect *src_dirty_rect, struct disp_rect *dst_dirty_rect,
	struct disp_layer_config *lyr_bak, struct disp_layer_config *lyr)
{
	switch (rot) {
	case ROTATION_SW_90:
		lyr->info.screen_win.x = scn_size.height
			 - lyr_bak->info.screen_win.y
			 - lyr_bak->info.screen_win.height;
		lyr->info.screen_win.y = lyr_bak->info.screen_win.x;
		lyr->info.screen_win.width = lyr_bak->info.screen_win.height;
		lyr->info.screen_win.height = lyr_bak->info.screen_win.width;
		lyr->info.fb.crop.x = 0;
		lyr->info.fb.crop.y = 0;
		lyr->info.fb.crop.width = lyr_bak->info.fb.crop.height;
		lyr->info.fb.crop.height = lyr_bak->info.fb.crop.width;
		lyr->info.fb.size[0].width = DISPALIGN(
			lyr_bak->info.fb.size[0].height, ROTATION_SW_ALIGN);
		lyr->info.fb.size[0].height = lyr_bak->info.fb.size[0].width;
		lyr->info.fb.size[1].width = DISPALIGN(
			lyr_bak->info.fb.size[1].height, ROTATION_SW_ALIGN);
		lyr->info.fb.size[1].height = lyr_bak->info.fb.size[1].width;
		lyr->info.fb.size[2].width = DISPALIGN(
			lyr_bak->info.fb.size[2].height, ROTATION_SW_ALIGN);
		lyr->info.fb.size[2].height = lyr_bak->info.fb.size[2].width;
		dst_dirty_rect->x = (lyr_bak->info.fb.crop.height >> 32)
			- src_dirty_rect->y - src_dirty_rect->height;
		dst_dirty_rect->y = src_dirty_rect->x;
		dst_dirty_rect->width = src_dirty_rect->height;
		dst_dirty_rect->height = src_dirty_rect->width;
		break;
	case ROTATION_SW_270:
		lyr->info.screen_win.x = lyr_bak->info.screen_win.y;
		lyr->info.screen_win.y = scn_size.width
			 - lyr_bak->info.screen_win.x
			 - lyr_bak->info.screen_win.width;
		lyr->info.screen_win.width = lyr_bak->info.screen_win.height;
		lyr->info.screen_win.height = lyr_bak->info.screen_win.width;
		lyr->info.fb.crop.x = 0;
		lyr->info.fb.crop.y = 0;
		lyr->info.fb.crop.width = lyr_bak->info.fb.crop.height;
		lyr->info.fb.crop.height = lyr_bak->info.fb.crop.width;
		lyr->info.fb.size[0].width = DISPALIGN(
			lyr_bak->info.fb.size[0].height, ROTATION_SW_ALIGN);
		lyr->info.fb.size[0].height = lyr_bak->info.fb.size[0].width;
		lyr->info.fb.size[1].width = DISPALIGN(
			lyr_bak->info.fb.size[1].height, ROTATION_SW_ALIGN);
		lyr->info.fb.size[1].height = lyr_bak->info.fb.size[1].width;
		lyr->info.fb.size[2].width = DISPALIGN(
			lyr_bak->info.fb.size[2].height, ROTATION_SW_ALIGN);
		lyr->info.fb.size[2].height = lyr_bak->info.fb.size[2].width;
		dst_dirty_rect->x = src_dirty_rect->y;
		dst_dirty_rect->y = (lyr_bak->info.fb.crop.width >> 32)
			- src_dirty_rect->x - src_dirty_rect->width;
		dst_dirty_rect->width = src_dirty_rect->height;
		dst_dirty_rect->height = src_dirty_rect->width;
		break;
	case ROTATION_SW_180:
	case ROTATION_SW_0:
	default:
		DE_WRN("fixme: rot=%d\n", rot);
		return -1;
	}

	return 0;
}

static int check_dirty_rect(struct disp_rect *dirty, struct disp_rect64 *crop)
{
	int fixed = 0;
	int dirty_end, crop_end;
	int crop_x = (int)(crop->x >> 32);
	int crop_y = (int)(crop->y >> 32);
	unsigned int crop_w = (unsigned int)(crop->width >> 32);
	unsigned int crop_h = (unsigned int)(crop->height >> 32);

#ifdef CONFIG_AW_DISP2_NEON_ROTATION
	if (dirty->width & 0x01)
		dirty->width += 1;

	if (dirty->height & 0x01)
		dirty->height += 1;
#endif

	dirty_end = dirty->x + dirty->width;
	crop_end = crop_x + crop_w;
	if (crop_x > dirty->x) {
		dirty->x = crop_x;
		fixed = 1;
	}
	if (dirty_end > crop_end) {
		dirty_end = crop_end;
		fixed = 1;
	}
	if (1 == fixed) {
		if (dirty_end > dirty->x) {
			dirty->width = dirty_end - dirty->x;
			fixed = 0;
		} else {
			fixed = -1;
			goto fix_fail;
		}
	}

	dirty_end = dirty->y + dirty->height;
	crop_end = crop_y + crop_h;
	if (crop_y > dirty->y) {
		dirty->y = crop_y;
		fixed = 1;
	}
	if (dirty_end > crop_end) {
		dirty_end = crop_end;
		fixed = 1;
	}
	if (1 == fixed) {
		if (dirty_end > dirty->y) {
			dirty->height = dirty_end - dirty->y;
			fixed = 0;
		} else {
			fixed = -1;
			goto fix_fail;
		}
	}

fix_fail:
	if (fixed) {
		DE_WRN("dirty[%d,%d,%d,%d],crop[%d,%d,%d,%d]\n",
			dirty->x, dirty->y, dirty->width,
			dirty->height, crop_h, crop_h, crop_w, crop_h);
	}

	return fixed;
}

static prv_data_t *
disp_rotation_sw_get_priv(struct disp_rotation_sw *rot_sw,
	unsigned int chn, unsigned int layer_id)
{
	int i;
	prv_data_t *p_rot_sw_p = rot_sw_p;

	if (NULL == rot_sw) {
		DE_WRN("NULL hdl!\n");
		return NULL;
	}
	for (i = 0; i < rot_sw->disp; i++) {
		p_rot_sw_p += bsp_disp_feat_get_num_layers(i);
	}
	for (i = 0; i < chn; i++) {
		p_rot_sw_p += bsp_disp_feat_get_num_layers_by_chn(
			rot_sw->disp, i);
	}
	p_rot_sw_p += layer_id;

	return p_rot_sw_p;
}

static s32 disp_rot_sw_apply(struct disp_rotation_sw *rot_sw,
	struct disp_layer_config *lyr_config, struct disp_rect src_dirty_rect)
{
	int ret = 0;
	struct disp_rect dst_dirty_rect;
	prv_data_t *p_rot_sw_p = NULL;
	unsigned int degree;
	unsigned int Bpp[3] = {0, 0, 0};

	if ((NULL == rot_sw) || (NULL == lyr_config)) {
		DE_INF("NULL hdl![%d,%d]\n", (int)rot_sw, (int)lyr_config);
		return -1;
	}

	if (DISP_FORMAT_BGRA_8888 < lyr_config->info.fb.format)
		return 0;

	p_rot_sw_p = disp_rotation_sw_get_priv(rot_sw,
		lyr_config->channel, lyr_config->layer_id);
	if (NULL == p_rot_sw_p) {
		DE_WRN("p_rot_sw_p is null !\n");
		return -1;
	}

	memcpy((void *)&p_rot_sw_p->lyr_bak, (void *)lyr_config,
		sizeof(struct disp_layer_config));

	degree = p_rot_sw_p->degree;
	switch (degree) {
	case ROTATION_SW_90:
	case ROTATION_SW_270:
		break;
	case ROTATION_SW_0:
		goto out;
	default:
		DE_WRN("not support this degree[%d]\n", degree);
		ret = -1;
		goto out;
	}
	switch (lyr_config->info.fb.format) {
	case DISP_FORMAT_ARGB_8888:
	case DISP_FORMAT_ABGR_8888:
	case DISP_FORMAT_RGBA_8888:
	case DISP_FORMAT_BGRA_8888:
		Bpp[0] = 4;
		break;
	case DISP_FORMAT_YUV420_SP_UVUV:
	case DISP_FORMAT_YUV420_SP_VUVU:
		Bpp[0] = 1;
		Bpp[1] = 2;
		break;
	case DISP_FORMAT_YUV420_P:
		Bpp[0] = Bpp[1] = Bpp[2] = 1;
		break;
	default:
		DE_WRN("not support this mode[%d]\n",
			lyr_config->info.fb.format);
		ret = -1;
		goto out;
	}
	ret = check_dirty_rect(&src_dirty_rect, &lyr_config->info.fb.crop);
	if (ret)
		goto out;
	ret = update_layer_info(degree, rot_sw->screen_size,
		&src_dirty_rect, &dst_dirty_rect,
		&p_rot_sw_p->lyr_bak, lyr_config);
	if (ret)
		goto out;
	ret = manager_rotate_buffer(&lyr_config->info.fb, &p_rot_sw_p->tg_buf);
	if (ret) {
		goto out;
	}
	if (1 == lyr_config->enable) {
		update_rotate_fb(p_rot_sw_p->tg_buf.r_buf, p_rot_sw_p->tg_buf.w_buf,
			Bpp[0], lyr_config->info.fb.size[0].width * Bpp[0],
			&dst_dirty_rect, &rot_sw->screen_size);
		rotate_fb(&(p_rot_sw_p->lyr_bak.info.fb),
			&src_dirty_rect, &(lyr_config->info.fb), &dst_dirty_rect,
			(void *)p_rot_sw_p->tg_buf.w_buf->vir_addr, degree);
		flush_user_range((long)p_rot_sw_p->tg_buf.w_buf->vir_addr,
			(long)((char *)p_rot_sw_p->tg_buf.w_buf->vir_addr + p_rot_sw_p->tg_buf.w_buf->size));
	}

out:
	return ret;
}

static s32 disp_rot_sw_checkout(struct disp_rotation_sw *rot_sw,
	struct disp_layer_config *lyr_config)
{
	prv_data_t *p_rot_sw_p = NULL;
	unsigned long flags;
	s32 type;
	static unsigned int need_clear_flag;
	int ret = 0;

	if ((NULL == rot_sw) || (NULL == lyr_config)) {
		DE_WRN("NULL hdl[%d,%d]!\n", (int)rot_sw, (int)lyr_config);
		return -1;
	}

	if (DISP_FORMAT_BGRA_8888 < lyr_config->info.fb.format) {
		return 0;
	}

	p_rot_sw_p = disp_rotation_sw_get_priv(rot_sw,
		lyr_config->channel, lyr_config->layer_id);
	if (NULL == p_rot_sw_p) {
		DE_WRN("null pointer\n");
		return -1;
	}

	type = bsp_disp_get_output_type(0);
	spin_lock_irqsave(&p_rot_sw_p->data_lock, flags);
	if (type == DISP_OUTPUT_TYPE_HDMI) {
		need_clear_flag = 1;
		memcpy((void *)&(lyr_config->info.fb.crop),
			(void *)&(p_rot_sw_p->lyr_bak.info.fb.crop), sizeof(struct disp_rect64));
		memcpy((void *)lyr_config->info.fb.size,
			(void *)p_rot_sw_p->lyr_bak.info.fb.size,
			sizeof(p_rot_sw_p->lyr_bak.info.fb.size));
		memcpy((void *)lyr_config->info.fb.addr,
			(void *)p_rot_sw_p->lyr_bak.info.fb.addr,
			sizeof(p_rot_sw_p->lyr_bak.info.fb.addr));
		spin_unlock_irqrestore(&p_rot_sw_p->data_lock, flags);
		return 0;
	} else {
		if (need_clear_flag == 1) {
			ret = 1;
			need_clear_flag = 0;
		}
		 if (p_rot_sw_p->degree == ROTATION_SW_90 ||
			p_rot_sw_p->degree == ROTATION_SW_270) {
			unsigned int x, y, width, height;

			x = lyr_config->info.screen_win.x;
			y = lyr_config->info.screen_win.y;
			width = lyr_config->info.screen_win.width;
			height = lyr_config->info.screen_win.height;
			lyr_config->info.screen_win.x = y;
			lyr_config->info.screen_win.y = x;
			lyr_config->info.screen_win.width = height;
			lyr_config->info.screen_win.height = width;
		}
		memcpy((void *)&(lyr_config->info.fb.crop),
			(void *)&(p_rot_sw_p->lyr_bak.info.fb.crop), sizeof(struct disp_rect64));
		memcpy((void *)lyr_config->info.fb.size,
			(void *)p_rot_sw_p->lyr_bak.info.fb.size,
			sizeof(p_rot_sw_p->lyr_bak.info.fb.size));
		memcpy((void *)lyr_config->info.fb.addr,
			(void *)p_rot_sw_p->lyr_bak.info.fb.addr,
			sizeof(p_rot_sw_p->lyr_bak.info.fb.addr));
	}
	spin_unlock_irqrestore(&p_rot_sw_p->data_lock, flags);
	return ret;
}

static s32 disp_rot_sw_set_layer_degree(struct disp_rotation_sw *rot_sw,
	u32 chn, u32 lyr_id, u32 degree)
{
	prv_data_t *p_rot_sw_p = NULL;
	p_rot_sw_p = disp_rotation_sw_get_priv(rot_sw, chn, lyr_id);
	if (NULL != p_rot_sw_p) {
		p_rot_sw_p->degree = degree;
		printk("set layer degree[%d] ok\n", degree);
		return 0;
	}
	DE_WRN("p_rot_sw_p is null !\n");
	return -1;
}

static s32 disp_rot_sw_get_layer_degree(struct disp_rotation_sw *rot_sw,
	u32 chn, u32 lyr_id)
{
	prv_data_t *p_rot_sw_p = NULL;
	p_rot_sw_p = disp_rotation_sw_get_priv(rot_sw, chn, lyr_id);
	if (NULL != p_rot_sw_p) {
		printk("get layer degree[%d] ok\n", p_rot_sw_p->degree);
		return p_rot_sw_p->degree;
	}
	DE_WRN("p_rot_sw_p is null !\n");
	return -1;
}

static s32 disp_rot_sw_set_manager(struct disp_rotation_sw *rot_sw,
	struct disp_manager *mgr)
{
	unsigned long flags;

	if ((NULL == rot_sw) || (NULL == mgr)) {
		DE_INF("NULL hdl!\n");
		return -1;
	}
	printk("%s,%d\n", __func__, __LINE__);
	spin_lock_irqsave(&s_data_lock, flags);
	rot_sw->manager = mgr;
	mgr->rot_sw = rot_sw;
	spin_unlock_irqrestore(&s_data_lock, flags);

	return 0;
}

static s32 disp_rot_sw_unset_manager(struct disp_rotation_sw *rot_sw)
{
	unsigned long flags;

	if ((NULL == rot_sw)) {
		DE_INF("NULL hdl!\n");
		return -1;
	}
	spin_lock_irqsave(&s_data_lock, flags);
	if (rot_sw->manager)
		rot_sw->manager->enhance = NULL;
	rot_sw->manager = NULL;
	spin_unlock_irqrestore(&s_data_lock, flags);

	return 0;
}

static s32 disp_rot_sw_init(struct disp_rotation_sw *rot_sw)
{
	char primary_key[32] = {0};
	char sub_name[32] = {0};
	int ret, i;
	int value = 0;
	int layer_num = 0;
	prv_data_t *p_rot_sw_p = NULL;

	if (NULL == rot_sw) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	p_rot_sw_p = disp_rotation_sw_get_priv(rot_sw, 0, 0);
	if (NULL == p_rot_sw_p) {
		return -1;
	}

	strncpy(primary_key, "disp", 32);
	sprintf(sub_name, "degree%d", rot_sw->disp);
	ret = disp_sys_script_get_item(primary_key, sub_name, &value, 1);
	if (ret != 1) {
		value = ROTATION_SW_0;
	}

	switch (value) {
	case ROTATION_SW_0:
	case ROTATION_SW_90:
	case ROTATION_SW_180:
	case ROTATION_SW_270:
		rot_sw->degree = value;
		break;
	default:
		rot_sw->degree = ROTATION_SW_0;
	}

	value = DISP_OUTPUT_TYPE_LCD;
	if (DISP_OUTPUT_TYPE_LCD == value) {
		sprintf(primary_key, "lcd%d", rot_sw->disp);
		strncpy(sub_name, "lcd_x", 32);
		ret = disp_sys_script_get_item(primary_key, sub_name, &value, 1);
		if (1 != ret) {
			DE_WRN("check me!");
			value = 0;
		}
		rot_sw->screen_size.width = value;
		strncpy(sub_name, "lcd_y", 32);
		ret = disp_sys_script_get_item(primary_key, sub_name, &value, 1);
		if (1 != ret) {
			DE_WRN("check me!");
			value = 0;
		}
		rot_sw->screen_size.height = value;
	} else {
		DE_WRN("fixme!");
		rot_sw->screen_size.width = bsp_disp_get_screen_width_from_output_type(
			rot_sw->disp, (u32)value, 0);
		rot_sw->screen_size.height = bsp_disp_get_screen_height_from_output_type(
			rot_sw->disp, (u32)value, 0);
	}
	if ((ROTATION_SW_90 == rot_sw->degree)
		|| (ROTATION_SW_270 == rot_sw->degree)) {
		rot_sw->screen_size.width ^= rot_sw->screen_size.height;
		rot_sw->screen_size.height ^= rot_sw->screen_size.width;
		rot_sw->screen_size.width ^= rot_sw->screen_size.height;
	}
	printk("***** disp_rot_sw_init:rot-degree=%d, frame[%d,%d] *****\n", rot_sw->degree,
		rot_sw->screen_size.width, rot_sw->screen_size.height);

	layer_num = bsp_disp_feat_get_num_layers(rot_sw->disp);
	strncpy(primary_key, "disp", 32);
	for (i = 0; i < layer_num; i++) {
		sprintf(sub_name, "degree%d_%d", rot_sw->disp, i);
		ret = disp_sys_script_get_item(primary_key, sub_name, &value, 1);
		if (ret != 1) {
			value = rot_sw->degree;
		}
		switch (value) {
		case ROTATION_SW_0:
		case ROTATION_SW_90:
		case ROTATION_SW_180:
		case ROTATION_SW_270:
			p_rot_sw_p->degree = value;
			break;
		default:
			p_rot_sw_p->degree = rot_sw->degree;
		}

	  #if defined(__LINUX_PLAT__)
		spin_lock_init(&p_rot_sw_p->data_lock);
	  #endif
		p_rot_sw_p++;
	}

	return 0;
}

static s32 disp_rot_sw_exit(struct disp_rotation_sw *rot_sw)
{
	return 0;
}

s32 disp_init_rotation_sw(struct disp_bsp_init_para *para)
{
	u32 num_screens = 0;
	u32 max_num_layers = 0;
	u32 disp;
	u32 used = 0;
	int ret;
	struct disp_rotation_sw *p_rot_sw = NULL;

	ret = disp_sys_script_get_item("disp", "disp_rotation_used", &used, 1);
	if (ret != 1) {
		used = 0;
	}
	if (!used) {
		printk("rotation_sw module is config as no used\n");
		return 0;
	}
	printk("disp_init_rotation_sw\n");


#if defined(__LINUX_PLAT__)
	spin_lock_init(&s_data_lock);
#endif
	num_screens = bsp_disp_feat_get_num_screens();
	g_rot_sw = (struct disp_rotation_sw *)kmalloc(
		sizeof(struct disp_rotation_sw) * num_screens, GFP_KERNEL | __GFP_ZERO);
	if (NULL == g_rot_sw) {
		DE_WRN("malloc memory fail for rotation_sw! size=0x%x\n",
			sizeof(struct disp_rotation_sw) * num_screens);
		return DIS_FAIL;
	}
	for (disp = 0; disp < num_screens; disp++) {
		max_num_layers += bsp_disp_feat_get_num_layers(disp);
	}
	rot_sw_p = (prv_data_t *)kmalloc(
		sizeof(prv_data_t) * max_num_layers, GFP_KERNEL | __GFP_ZERO);
	if (NULL == rot_sw_p) {
		DE_WRN("malloc memory fail! size=0x%x\n",
			sizeof(prv_data_t) * max_num_layers);
		return DIS_FAIL;
	}

	p_rot_sw = g_rot_sw;
	for (disp = 0; disp < num_screens; disp++) {
		p_rot_sw->disp = disp;
		p_rot_sw->init = disp_rot_sw_init;
		p_rot_sw->exit = disp_rot_sw_exit;
		p_rot_sw->set_layer_degree = disp_rot_sw_set_layer_degree;
		p_rot_sw->get_layer_degree = disp_rot_sw_get_layer_degree;
		p_rot_sw->set_manager = disp_rot_sw_set_manager;
		p_rot_sw->unset_manager = disp_rot_sw_unset_manager;
		p_rot_sw->apply = disp_rot_sw_apply;
		p_rot_sw->checkout = disp_rot_sw_checkout;

		disp_rot_sw_init(p_rot_sw);
		p_rot_sw++;
	}

	return 0;
}

