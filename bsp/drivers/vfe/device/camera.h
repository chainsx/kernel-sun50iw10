/*
 * linux-4.9/drivers/media/platform/sunxi-vfe/device/camera.h
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

/*
 * sunxi sensor header file
 * Author:raymonxiu
 */
#ifndef __CAMERA__H__
#define __CAMERA__H__

#include <media/v4l2-subdev.h>
#include <linux/videodev2.h>
#include "../vfe.h"
#include "../vfe_subdev.h"
#include "../csi_cci/cci_helper.h"
#include "camera_cfg.h"
#include "../platform_cfg.h"
/*
 * Basic window sizes.  These probably belong somewhere more globally
 * useful.
 */
#define ABS_SENSOR(x)                 ((x) > 0 ? (x) : -(x))

#define HXGA_WIDTH    4000
#define HXGA_HEIGHT   3000
#define QUXGA_WIDTH   3264
#define QUXGA_HEIGHT  2448
#define QSXGA_WIDTH   2592
#define QSXGA_HEIGHT  1936
#define QXGA_WIDTH    2048
#define QXGA_HEIGHT   1536
#define HD1080_WIDTH  1920
#define HD1080_HEIGHT 1080
#define UXGA_WIDTH    1600
#define UXGA_HEIGHT   1200
#define SXGA_WIDTH    1280
#define SXGA_HEIGHT   960
#define HD720_WIDTH   1280
#define HD720_HEIGHT  720
#define XGA_WIDTH     1024
#ifdef CONFIG_ARCH_SUN3IW1P1
#define XGA_HEIGHT    576
#else
#define XGA_HEIGHT    768
#endif
#define SVGA_WIDTH    800
#define SVGA_HEIGHT   600
#define VGA_WIDTH     640
#define VGA_HEIGHT    480
#define QVGA_WIDTH    320
#define QVGA_HEIGHT   240
#define CIF_WIDTH     352
#define CIF_HEIGHT    288
#define QCIF_WIDTH    176
#define QCIF_HEIGHT   144

#define CSI_GPIO_HIGH     1
#define CSI_GPIO_LOW     0
#define CCI_BITS_8           8
#define CCI_BITS_16         16

struct sensor_info {
	struct v4l2_subdev                    sd;
	struct mutex                          lock;
	struct sensor_format_struct           *fmt;  /* Current format */
	struct sensor_format_struct           *fmt_pt;	/* format start */
	struct sensor_win_size                *win_pt;		/* win start */
	unsigned int                          fmt_num;
	unsigned int                          win_size_num;
	unsigned int                          sensor_field;
	enum standby_mode                     stby_mode;
	struct v4l2_ctrl_handler	          handler;
	unsigned int                          width;
	unsigned int                          height;
	unsigned int                          capture_mode;   /* V4L2_MODE_VIDEO/V4L2_MODE_IMAGE */
	unsigned int                          af_first_flag;
	unsigned int                          init_first_flag;
	unsigned int                          preview_first_flag;
	unsigned int                          auto_focus;  /* 0:not in contin_focus 1: contin_focus */
	unsigned int                          focus_status;   /* 0:idle 1:busy */
	unsigned int                          low_speed;    /* 0:high speed 1:low speed */
	int                                   brightness;
	int                                   contrast;
	int                                   saturation;
	int                                   hue;
	unsigned int                          hflip;
	unsigned int                          vflip;
	unsigned int                          gain;
	unsigned int                          autogain;
	unsigned int                          exp;
	int                                   exp_bias;
	enum v4l2_exposure_auto_type          autoexp;
	unsigned int                          autowb;
	enum v4l2_auto_n_preset_white_balance wb;
	enum v4l2_colorfx                     clrfx;
	enum v4l2_flash_led_mode              flash_mode;
	enum v4l2_power_line_frequency        band_filter;
	/* enum v4l2_autofocus_ctrl af_ctrl; */
	struct v4l2_fract                     tpf;
	struct sensor_win_size                *current_wins;
	struct flash_dev_info                 *fl_dev_info;
	struct delayed_work work;
	struct workqueue_struct               *wq;
	int night_mode;
	int streaming;
};

struct sensor_format_struct {
	__u8 *desc;
	u32 mbus_code;
	struct regval_list *regs;
	int regs_size;
	int bpp; /* Bytes per pixel */
};

#endif /* __CAMERA__H__ */
