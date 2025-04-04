/*
 * include/media/sunxi_camera.h -- Ctrl IDs definitions for sunxi-vfe
 *
 * Copyright (C) 2014 Allwinnertech Co., Ltd.
 * Copyright (C) 2015 Yang Feng
 *
 * Author: Yang Feng <yangfeng@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 */
#ifndef _SUNXI_CAMERA_H_
#define _SUNXI_CAMERA_H_

#include <linux/types.h>
#include <linux/videodev2.h>


/*  Flags for 'capability' and 'capturemode' fields */
#define V4L2_MODE_HIGHQUALITY		0x0001
#define V4L2_MODE_VIDEO				0x0002
#define V4L2_MODE_IMAGE				0x0003
#define V4L2_MODE_PREVIEW			0x0004
/*
 *	USER CIDS
 */
struct v4l2_win_coordinate {
	__s32			x1;
	__s32			y1;
	__s32			x2;
	__s32			y2;
};

#define V4L2_MAX_WIN_NUM	10

#define V4L2_FLASH_LED_MODE_AUTO				(V4L2_FLASH_LED_MODE_TORCH+1)
#define V4L2_FLASH_LED_MODE_RED_EYE				(V4L2_FLASH_LED_MODE_TORCH+2)

struct v4l2_win_setting {
	__s32 win_num;
	struct v4l2_win_coordinate coor[V4L2_MAX_WIN_NUM];
};

enum v4l2_gain_shift {
	V4L2_GAIN_SHIFT	= 0,
	V4L2_SHARP_LEVEL_SHIFT	= 8,
	V4L2_SHARP_MIN_SHIFT	= 20,
	V4L2_NDF_SHIFT	= 26,
};

#define MAX_EXP_FRAMES     5

/* The base for the sunxi-vfe controls. Total of 64 controls is reserved for this driver, add by yangfeng */
#define V4L2_CID_USER_SUNXI_CAMERA_BASE		(V4L2_CID_USER_BASE + 0x1050)

#define V4L2_CID_HFLIP_THUMB					(V4L2_CID_USER_SUNXI_CAMERA_BASE+0)
#define V4L2_CID_VFLIP_THUMB					(V4L2_CID_USER_SUNXI_CAMERA_BASE+1)
#define V4L2_CID_AUTO_FOCUS_INIT				(V4L2_CID_USER_SUNXI_CAMERA_BASE+2)
#define V4L2_CID_AUTO_FOCUS_RELEASE				(V4L2_CID_USER_SUNXI_CAMERA_BASE+3)
#define V4L2_CID_GSENSOR_ROTATION				(V4L2_CID_USER_SUNXI_CAMERA_BASE+4)
#define V4L2_CID_FRAME_RATE			(V4L2_CID_USER_SUNXI_CAMERA_BASE+5)

enum v4l2_take_picture {
	V4L2_TAKE_PICTURE_STOP	= 0,
	V4L2_TAKE_PICTURE_NORM	= 1,
	V4L2_TAKE_PICTURE_FAST	= 2,
	V4L2_TAKE_PICTURE_FLASH	= 3,
	V4L2_TAKE_PICTURE_HDR	= 4,
};
struct isp_hdr_setting_t {
	__s32 hdr_en;
	__s32 hdr_mode;
	__s32 frames_count;
	__s32 total_frames;
	__s32 values[MAX_EXP_FRAMES];
};

#define HDR_CTRL_GET    0
#define HDR_CTRL_SET     1
struct isp_hdr_ctrl {
	__s32 flag;
	__s32 count;
	struct isp_hdr_setting_t hdr_t;
};

#define V4L2_CID_TAKE_PICTURE		(V4L2_CID_USER_SUNXI_CAMERA_BASE+6)

typedef union {
	unsigned int dwval;
	struct {
		unsigned int af_sharp		:  16;
		unsigned int hdr_cnt		:  4;
		unsigned int flash_ok		:  1;
		unsigned int capture_ok		:  1;
		unsigned int fast_capture_ok	:  1;
		unsigned int res0		:  9;
	} bits;
} IMAGE_FLAG_t;

#define  V4L2_CID_HOR_VISUAL_ANGLE	(V4L2_CID_USER_SUNXI_CAMERA_BASE+7)
#define  V4L2_CID_VER_VISUAL_ANGLE	(V4L2_CID_USER_SUNXI_CAMERA_BASE+8)
#define  V4L2_CID_FOCUS_LENGTH		(V4L2_CID_USER_SUNXI_CAMERA_BASE+9)
#define  V4L2_CID_R_GAIN		(V4L2_CID_USER_SUNXI_CAMERA_BASE+10)
#define  V4L2_CID_G_GAIN		(V4L2_CID_USER_SUNXI_CAMERA_BASE+11)
#define  V4L2_CID_B_GAIN		(V4L2_CID_USER_SUNXI_CAMERA_BASE+12)

enum v4l2_sensor_type {
	V4L2_SENSOR_TYPE_YUV		= 0,
	V4L2_SENSOR_TYPE_RAW		= 1,
};

#define V4L2_CID_SENSOR_TYPE			(V4L2_CID_USER_SUNXI_CAMERA_BASE+13)

/*
 *	PRIVATE IOCTRLS
 */

struct isp_stat_buf {
	void __user *buf;
	__u32 buf_size;
};
struct isp_exif_attribute {
	struct v4l2_fract exposure_time;
	struct v4l2_fract shutter_speed;
	__u32 fnumber;
	__u32 focal_length;
	__s32 exposure_bias;
	__u32 iso_speed;
	__u32 flash_fire;
	__u32 brightness;
	__s32 reserved[16];
};

struct rot_channel_cfg {
	__u32 sel_ch;
	__u32 rotation;
	struct v4l2_pix_format pix;
};
#define VIDIOC_ISP_AE_STAT_REQ \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 1, struct isp_stat_buf)
#define VIDIOC_ISP_HIST_STAT_REQ \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 2, struct isp_stat_buf)
#define VIDIOC_ISP_AF_STAT_REQ \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 3, struct isp_stat_buf)
#define VIDIOC_ISP_EXIF_REQ \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 4, struct isp_exif_attribute)
#define VIDIOC_ISP_GAMMA_REQ \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 5, struct isp_stat_buf)
#define VIDIOC_AUTO_FOCUS_WIN \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 6, struct v4l2_win_setting)
#define VIDIOC_AUTO_EXPOSURE_WIN \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 7, struct v4l2_win_setting)
#define VIDIOC_HDR_CTRL \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 8, struct isp_hdr_ctrl)

#define VIDIOC_SET_SUBCHANNEL \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 9, struct v4l2_pix_format)
#define VIDIOC_SET_ROTCHANNEL \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 10, struct rot_channel_cfg)


#endif /* _SUNXI_CAMERA_H_ */
