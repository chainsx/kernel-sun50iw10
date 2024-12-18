/*
 * g2d_driver_i/g2d_driver_i.h
 *
 * Copyright (c) 2007-2019 Allwinnertech Co., Ltd.
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
#ifndef __G2D_DRIVER_I_H
#define __G2D_DRIVER_I_H

#include "g2d_bsp.h"

/* #include "g2d_bsp_v2.h" */

#define INFO(format, args...) pr_info("%s: " format, "G2D", ## args)

#if defined(CONFIG_AW_FPGA_S4) || defined(CONFIG_AW_FPGA_V7)
#define WAIT_CMD_TIME_MS 500
#else
#define WAIT_CMD_TIME_MS 100
#endif


#define G2DALIGN(value, align) ((align == 0) ? \
				value : \
				(((value) + ((align) - 1)) & ~((align) - 1)))
#if defined(CONFIG_ARCH_SUN8IW20) || defined(CONFIG_ARCH_SUN20IW1)
#define G2D_IOMMU_MASTER_ID 3
#endif

struct dmabuf_item {
	struct list_head list;
	int fd;
	struct dma_buf *buf;
	struct dma_buf_attachment *attachment;
	struct sg_table *sgt;
	dma_addr_t dma_addr;
	unsigned long long id;
};

struct info_mem {
	unsigned long phy_addr;
	void *virt_addr;
	__u32 b_used;
	__u32 mem_len;
};

typedef struct {
	struct device *dev;
	struct resource *mem;
	void __iomem *io;
	__u32 irq;
	struct mutex mutex;
	struct clk *clk;
	bool opened;
	__u32 user_cnt;
	struct clk *clk_parent;
	struct clk *bus_clk;
	struct clk *mbus_clk;
	struct reset_control *reset;
} __g2d_info_t;

typedef struct {
	__u32 mid;
	__u32 used;
	__u32 status;
	struct semaphore *g2d_finished_sem;
	struct semaphore *event_sem;
	wait_queue_head_t queue;
	__u32 finish_flag;
} __g2d_drv_t;

struct g2d_alloc_struct {
	__u32 address;
	__u32 size;
	__u32 u_size;
	struct g2d_alloc_struct *next;
};

/* g2d_format_attr - g2d format attribute
 *
 * @format: pixel format
 * @bits: bits of each component
 * @hor_rsample_u: reciprocal of horizontal sample rate
 * @hor_rsample_v: reciprocal of horizontal sample rate
 * @ver_rsample_u: reciprocal of vertical sample rate
 * @hor_rsample_v: reciprocal of vertical sample rate
 * @uvc: 1: u & v component combined
 * @interleave: 0: progressive, 1: interleave
 * @factor & div: bytes of pixel = factor / div (bytes)
 * @addr[out]: address for each plane
 * @trd_addr[out]: address for each plane of right eye buffer
 */
struct g2d_format_attr {
	g2d_fmt_enh format;
	unsigned int bits;
	unsigned int hor_rsample_u;
	unsigned int hor_rsample_v;
	unsigned int ver_rsample_u;
	unsigned int ver_rsample_v;
	unsigned int uvc;
	unsigned int interleave;
	unsigned int factor;
	unsigned int div;
};


int g2d_wait_cmd_finish(unsigned int timeout);
int g2d_dma_map(int fd, struct dmabuf_item *item);
void g2d_dma_unmap(struct dmabuf_item *item);
__s32 g2d_set_info(g2d_image_enh *g2d_img, struct dmabuf_item *item);
void *g2d_malloc(__u32 bytes_num, __u32 *phy_addr);
void g2d_free(void *virt_addr, void *phy_addr, unsigned int size);
__s32 g2d_image_check(g2d_image_enh *p_image);
__s32 g2d_byte_cal(__u32 format, __u32 *ycnt, __u32 *ucnt, __u32 *vcnt);
__u32 cal_align(__u32 width, __u32 align);

#endif /* __G2D_DRIVER_I_H */
