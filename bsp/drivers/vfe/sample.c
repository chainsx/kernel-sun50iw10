/*
 * linux-4.9/drivers/media/platform/sunxi-vfe/sample.c
 *
 * Copyright (c) 2007-2017 Allwinnertech Co., Ltd.
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

#include <linux/err.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>

#include "vfe.h"

extern int vfe_open_special(int id);
extern int vfe_close_special(int id);
extern int vfe_s_input_special(int id, int sel);
extern int vfe_s_fmt_special(int id, struct v4l2_format *f);
extern int vfe_g_fmt_special(int id, struct v4l2_format *f);
extern int vfe_dqbuffer_special(int id, struct vfe_buffer **buf);
extern int vfe_qbuffer_special(int id, struct vfe_buffer *buf);
extern int vfe_streamon_special(int id, enum v4l2_buf_type i);
extern int vfe_streamoff_special(int id, enum v4l2_buf_type i);
extern void vfe_register_buffer_done_callback(int id, void *func);
extern int os_mem_alloc_sample(int id, struct vfe_mm *mem_man);
extern void os_mem_free_sample(int id, struct vfe_mm *mem_man);

#define BUF_NUM 8
#define ALIGN_4K(x) (((x) + (4095)) & ~(4095))
#define ALIGN_16B(x) (((x) + (15)) & ~(15))

int vfe_id;
int width = 640;
int height = 480;
int frame_cnt;
int save_cnt;

struct work_struct save_work;
struct vfe_mm ion_buf[BUF_NUM];
struct vfe_buffer video_buf[BUF_NUM];
char file_path_isp[100] = "/mnt/fb.yuv";

module_param(width, uint, S_IRUGO | S_IWUSR);
module_param(height, uint, S_IRUGO | S_IWUSR);
module_param(vfe_id, uint, S_IRUGO | S_IWUSR);
module_param(save_cnt, uint, S_IRUGO | S_IWUSR);
module_param_string(file_path_isp, file_path_isp, sizeof(file_path_isp), S_IRUGO | S_IWUSR);

static int file_write(struct file *fp, char *buf, size_t len)
{
	mm_segment_t old_fs;
	loff_t pos = 0;
	int buf_len;

	if (IS_ERR_OR_NULL(fp))	{
		pr_info("cfg write file error, fp is null!");
		return -1;
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	buf_len = kernel_write(fp, buf, len, &pos);
	set_fs(old_fs);

	if (buf_len < 0)
		return -1;
	if (buf_len != len)
		pr_info("buf_len = %x, len = %pa\n", buf_len, &len);
	return buf_len;
}

static void sample_save_frame(struct work_struct *work)
{
	struct file *fp;
	int i, ret;
	struct vfe_buffer *vfe_buf;

	vfe_dqbuffer_special(vfe_id, &vfe_buf);

	fp = filp_open(file_path_isp, O_RDWR | O_APPEND | O_CREAT, 0666);
	if (IS_ERR_OR_NULL(fp)) {
		pr_info("open %s failed!, ERR NO is %ld.\n", file_path_isp, (long)fp);
		goto qbuf;
	}
	for (i = 0; i < BUF_NUM; ++i) {
		if (vfe_buf->paddr == ion_buf[i].dma_addr)
			break;
	}

	if (i == 8) {
		pr_info("no match ion_buf with vfe_buf!\n");
		return -1;
	}

	ret = file_write(fp, (char *)ion_buf[i].vir_addr, width*height*3/2);
	if (ret < 0)
		pr_info("file_write failed!, ERR NO is %d.\n", ret);

	ret = filp_close(fp, NULL);
	if (ret < 0)
		pr_info("file close failed!, ERR NO is %d.\n", ret);
qbuf:
	vfe_qbuffer_special(vfe_id, vfe_buf);
}

static int sample_req_buffers(void)
{
	unsigned int i;

	for (i = 0; i < BUF_NUM; ++i) {
		ion_buf[i].size = ALIGN_4K(ALIGN_16B(width)*height*3/2);
		if (os_mem_alloc_sample(vfe_id, &ion_buf[i]) < 0) {
			pr_info("alloc ion buffer failed\n");
			return -1;
		}
		video_buf[i].paddr = ion_buf[i].dma_addr;

		if (vfe_qbuffer_special(vfe_id, &video_buf[i])) {
			pr_info("vfe_qbuffer_special failed\n");
			return -1;
		}
	}
	return 0;

}

static int sample_free_buffers(void)
{
	unsigned int i;
	struct vfe_buffer *vfe_buf;

	for (i = 0; i < BUF_NUM; ++i) {
		os_mem_free_sample(vfe_id, &ion_buf[i]);
		vfe_dqbuffer_special(vfe_id, &vfe_buf);
	}
	return 0;
}

static int sample_video_open(int sel)
{
	if (vfe_open_special(vfe_id) < 0) {
		pr_info("vfe_open_special falied\n");
		return -1;
	}

	if (vfe_s_input_special(vfe_id, 0) < 0) {
		pr_info("vfe_s_input_special %d error!\n", sel);
		return -1;
	}

	return 0;
}

static int sample_fmt_set(void)
{
	struct v4l2_format fmt;

	memset(&fmt, 0, sizeof(fmt));
	fmt.type			= V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width	= width;
	fmt.fmt.pix.height	= height;
	fmt.fmt.pix.pixelformat	= V4L2_PIX_FMT_YUV420;
	fmt.fmt.pix.field	= V4L2_FIELD_NONE;

	if (vfe_s_fmt_special(vfe_id, &fmt) < 0) {
		pr_info("vfe_s_fmt_special error!\n");
		return -1;
	}

	if (vfe_g_fmt_special(vfe_id, &fmt) < 0) {
		pr_info("vfe_g_fmt_special error!\n");
		return -1;
	} else {
		pr_info("resolution got from sensor = %d*%d\n",
			fmt.fmt.pix.width, fmt.fmt.pix.height);
	}
	return 0;
}

void sample_buffer_process(int id)
{
	struct vfe_buffer *vfe_buf;

	frame_cnt++;
	if (frame_cnt <= save_cnt) {
		pr_info("save frame %d\n", frame_cnt);
		schedule_work(&save_work);
	} else {
		vfe_dqbuffer_special(vfe_id, &vfe_buf);
		pr_info("csi %d buffer process\n", id);
		vfe_qbuffer_special(vfe_id, vfe_buf);
	}
}

int sample_start(void)
{
	if (-1 == sample_video_open(0))
		return -1;
	if (-1 == sample_fmt_set())
		return -1;
	if (-1 == sample_req_buffers())
		return -1;

	vfe_register_buffer_done_callback(vfe_id, sample_buffer_process);

	frame_cnt = 0;

	if (vfe_streamon_special(vfe_id, V4L2_BUF_TYPE_VIDEO_CAPTURE) < 0) {
		pr_info("vfe_streamon_special failed\n");
		return -1;
	} else {
		pr_info("vfe_streamon_special ok\n");
	}

	return 0;
}

int sample_end(void)
{
	if (vfe_streamoff_special(vfe_id, V4L2_BUF_TYPE_VIDEO_CAPTURE) < 0) {
		pr_info("vfe_streamoff_special failed\n");
		return -1;
	} else {
		pr_info("vfe_streamoff_special ok\n");
	}

	if (-1 == sample_free_buffers())
		return -1;

	if (vfe_close_special(vfe_id) < 0) {
		pr_info("vfe_close_special falied\n");
		return -1;
	}

	return 0;
}

static int __init sample_init(void)
{
	INIT_WORK(&save_work, sample_save_frame);

	return sample_start();
}

static void __exit sample_exit(void)
{
	flush_work(&save_work);
	sample_end();
}

module_init(sample_init);
module_exit(sample_exit);

MODULE_AUTHOR("zhaowei");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("video kernel api test");
