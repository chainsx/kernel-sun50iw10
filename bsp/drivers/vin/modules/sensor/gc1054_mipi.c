/*
 * A V4L2 driver for Raw cameras.
 *
 * Copyright (c) 2017 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Zhao Wei <zhaowei@allwinnertech.com>
 *    Liang WeiJie <liangweijie@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/videodev2.h>
#include <linux/clk.h>
#include <media/v4l2-device.h>
#include <media/v4l2-mediabus.h>
#include <linux/io.h>

#include "camera.h"
#include "sensor_helper.h"

MODULE_AUTHOR("hzh");
MODULE_DESCRIPTION("A low-level driver for gc1054 sensors");
MODULE_LICENSE("GPL");

#define MCLK              (24*1000*1000)
#define VREF_POL          V4L2_MBUS_VSYNC_ACTIVE_LOW
#define HREF_POL          V4L2_MBUS_HSYNC_ACTIVE_HIGH
#define CLK_POL           V4L2_MBUS_PCLK_SAMPLE_FALLING
#define V4L2_IDENT_SENSOR  0x1054

#define EXP_HIGH		0xff
#define EXP_MID			0x03
#define EXP_LOW			0x04
#define GAIN_HIGH		0xff
#define GAIN_LOW		0x24

#define ID_REG_HIGH		0xf0
#define ID_REG_LOW		0xf1
#define ID_VAL_HIGH		((V4L2_IDENT_SENSOR) >> 8)
#define ID_VAL_LOW		((V4L2_IDENT_SENSOR) & 0xff)


/* Set Gain */
#define ANALOG_GAIN_1 64 /* 1.00x */
#define ANALOG_GAIN_2 91 /* 1.42x  //wq:Refer to gc1024 Find the numbers in the gc1064 AEC Mechanism Instructions for Use */
#define ANALOG_GAIN_3 127/* 1.99x */
#define ANALOG_GAIN_4 182/* 2.85x */
#define ANALOG_GAIN_5 258/* 4.03x */
#define ANALOG_GAIN_6 369/* 5.77x */
#define ANALOG_GAIN_7 516/* 8.06x */
#define ANALOG_GAIN_8 738/* 11.53x */
#define ANALOG_GAIN_9 1032/* 16.12x */
#define ANALOG_GAIN_10 1491/* 23.3x */
#define ANALOG_GAIN_11 2084/* 32.57x */

/*
 * Our nominal (default) frame rate.
 */
#define SENSOR_FRAME_RATE 20

#define DBG_INFO(format, args...) (printk("[gc1054 MIPI INFO] LINE:%04d-->%s:"format, __LINE__, __func__, ##args))
#define DBG_ERR(format, args...)  (printk("[gc1054 MIPI ERR] LINE:%04d-->%s:"format, __LINE__, __func__, ##args))

/*
 * The gc1054 i2c address
 */
#define I2C_ADDR 0x42

#define SENSOR_NUM 0x2
#define SENSOR_NAME "gc1054_mipi"
#define SENSOR_NAME_2 "gc1054_mipi_2"

static int flip_status = 0xc3;

/*
 * The default register settings
 */

static struct regval_list sensor_default_regs[] = {

};

static struct regval_list sensor_720p20_regs[] = {

//////////////////////   SYS   //////////////////////
	{0xf2, 0x00},
	{0xf6, 0x00},
	{0xfc, 0x04},
	{0xf7, 0x01},
	{0xf8, 0x0c},
	{0xf9, 0x06},
	{0xfa, 0x80},
	{0xfc, 0x4e},
/////    ANALOG & CISCTL   ////////////////
	{0xfe, 0x00},
	{0x03, 0x02},
	{0x04, 0xa6},
	{0x05, 0x02},/* HB */
	{0x06, 0x07},
	{0x07, 0x01},/* VB */
	{0x08, 0x81},
	{0x09, 0x00},
	{0x0a, 0x04},/* row start */
	{0x0b, 0x00},
	{0x0c, 0x00},/* col start */
	{0x0d, 0x02},
	{0x0e, 0xd4},/* height 724 */
	{0x0f, 0x05},
	{0x10, 0x08},/* width 1288 */
	{0x17, 0xc0},
	{0x18, 0x02},
	{0x19, 0x08},
	{0x1a, 0x18},
	{0x1d, 0x12},
	{0x1e, 0x50},
	{0x1f, 0x80},
	{0x21, 0x30},
	{0x23, 0xf8},
	{0x25, 0x10},
	{0x28, 0x20},
	{0x34, 0x08}, /* data low */
	{0x3c, 0x10},
	{0x3d, 0x0e},
	{0xcc, 0x8e},
	{0xcd, 0x9a},
	{0xcf, 0x70},
	{0xd0, 0xa9},
	{0xd1, 0xc5},
	{0xd2, 0xed},/* data high */
	{0xd8, 0x3c},/* dacin offset */
	{0xd9, 0x7a},
	{0xda, 0x12},
	{0xdb, 0x50},
	{0xde, 0x0c},
	{0xe3, 0x60},
	{0xe4, 0x78},
	{0xfe, 0x01},
	{0xe3, 0x01},
	{0xe6, 0x10},/* ramps offset */
////// /////   ISP   //////////////////////
	{0xfe, 0x01},
	{0x80, 0x50},
	{0x88, 0x73},
	{0x89, 0x03},
	{0x90, 0x01},
	{0x92, 0x02},/* crop win 2<=y<=4 */
	{0x94, 0x03},/* crop win 2<=x<=5 */
	{0x95, 0x02},/* crop win height */
	{0x96, 0xd0},
	{0x97, 0x05},/* crop win width */
	{0x98, 0x00},
////// /////   BLK   //////////////////////
	{0xfe, 0x01},
	{0x40, 0x22},
	{0x43, 0x03},
	{0x4e, 0x3c},
	{0x4f, 0x00},
	{0x60, 0x00},
	{0x61, 0x80},
////// /////   GAIN   /////////////////////
	{0xfe, 0x01},
	{0xb0, 0x48},
	{0xb1, 0x01},
	{0xb2, 0x00},
	{0xb6, 0x00},
	{0xfe, 0x02},
	{0x01, 0x00},
	{0x02, 0x01},
	{0x03, 0x02},
	{0x04, 0x03},
	{0x05, 0x04},
	{0x06, 0x05},
	{0x07, 0x06},
	{0x08, 0x0e},
	{0x09, 0x16},
	{0x0a, 0x1e},
	{0x0b, 0x36},
	{0x0c, 0x3e},
	{0x0d, 0x56},
	{0xfe, 0x02},
	{0xb0, 0x00},/* col_gain[11:8] */
	{0xb1, 0x00},
	{0xb2, 0x00},
	{0xb3, 0x11},
	{0xb4, 0x22},
	{0xb5, 0x54},
	{0xb6, 0xb8},
	{0xb7, 0x60},
	{0xb9, 0x00},/* col_gain[12] */
	{0xba, 0xc0},
	{0xc0, 0x20},/* col_gain[7:0] */
	{0xc1, 0x2d},
	{0xc2, 0x40},
	{0xc3, 0x5b},
	{0xc4, 0x80},
	{0xc5, 0xb5},
	{0xc6, 0x00},
	{0xc7, 0x6a},
	{0xc8, 0x00},
	{0xc9, 0xd4},
	{0xca, 0x00},
	{0xcb, 0xa8},
	{0xcc, 0x00},
	{0xcd, 0x50},
	{0xce, 0x00},
	{0xcf, 0xa1},
////// ///   DARKSUN   ////////////////////
	{0xfe, 0x02},
	{0x54, 0xf7},
	{0x55, 0xf0},
	{0x56, 0x00},
	{0x57, 0x00},
	{0x58, 0x00},
	{0x5a, 0x04},
////// //////   DD   //////////////////////
	{0xfe, 0x04},
	{0x81, 0x8a},
////// /////	 MIPI	/////////////////////
	{0xfe, 0x03},
	{0x01, 0x03},
	{0x02, 0x11},
	{0x03, 0x90},
	{0x10, 0x90},
	{0x11, 0x2b},
	{0x12, 0x40}, /* /lwc 1280*5/4 */
	{0x13, 0x06},
	{0x15, 0x00},
	{0x21, 0x02},
	{0x22, 0x02},
	{0x23, 0x08},
	{0x24, 0x02},
	{0x25, 0x10},
	{0x26, 0x04},
	{0x29, 0x03},
	{0x2a, 0x02},
	{0x2b, 0x08},
	{0xfe, 0x00},

	{0xfe, 0x01}, /* RGB */
	{0x92, 0x03},/* crop win 2<=y<=4 */
	{0x94, 0x02},/* crop win 2<=x<=5 */
	{0xfe, 0x00},
};


static struct regval_list sensor_fmt_raw[] = {
};

static int sensor_g_exp(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);
	*value = info->exp;
	sensor_dbg("sensor_get_exposure = %d\n", info->exp);
	return 0;
}

static int sensor_s_exp(struct v4l2_subdev *sd, unsigned int exp_val)
{
	unsigned char explow, expmid, exphigh;
	struct sensor_info *info = to_state(sd);
	/* printk("------------%s  %d !!!!!!!\n", __func__, exp_val); */

	sensor_write(sd, 0xfe, 0x00);


#ifdef FRACTION_EXP
	exphigh = (unsigned char)((0x0f0000 & exp_val) >> 16);
	expmid = (unsigned char)((0x00ff00 & exp_val) >> 8);
	explow = (unsigned char)((0x0000ff & exp_val));
#else
	exphigh = 0;
	expmid = (unsigned char)((0x0ff000 & exp_val) >> 12);
	explow = (unsigned char)((0x000ff0 & exp_val) >> 4);
#endif

	sensor_write(sd, EXP_MID, expmid);
	sensor_write(sd, EXP_LOW, explow);


	info->exp = exp_val;
	return 0;
}

static int sensor_g_gain(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);
	*value = info->gain;
	sensor_dbg("sensor_get_gain = %d\n", info->gain);
	return 0;
}
static unsigned int t_gain;
static int sensor_s_gain(struct v4l2_subdev *sd, int gain_val)
{
	if (t_gain == gain_val)
		return 0;
	else
		t_gain = gain_val;

	unsigned char tmp;
	struct sensor_info *info = to_state(sd);
	gain_val = gain_val * 6;

	sensor_write(sd, 0xfe, 0x01);
	sensor_write(sd, 0xb1, 0x01);
	sensor_write(sd, 0xb2, 0x00);

	if (gain_val < 0x40)
		gain_val = 0x40;
	else if ((ANALOG_GAIN_1 <= gain_val) && (gain_val < ANALOG_GAIN_2)) {

		sensor_write(sd, 0xb6, 0x00);
		tmp = gain_val;
		sensor_write(sd, 0xb1, tmp >> 6);
		sensor_write(sd, 0xb2, (tmp << 2) & 0xfc);
	} else if ((ANALOG_GAIN_2 <= gain_val) && (gain_val < ANALOG_GAIN_3)) {

		sensor_write(sd, 0xb6, 0x01);
		tmp = 64 * gain_val / ANALOG_GAIN_2;
		sensor_write(sd, 0xb1, tmp >> 6);
		sensor_write(sd, 0xb2, (tmp << 2) & 0xfc);
	} else if ((ANALOG_GAIN_3 <= gain_val) && (gain_val < ANALOG_GAIN_4)) {

		sensor_write(sd, 0xb6, 0x02);
		tmp = 64 * gain_val / ANALOG_GAIN_3;
		sensor_write(sd, 0xb1, tmp >> 6);
		sensor_write(sd, 0xb2, (tmp << 2) & 0xfc);
	} else if ((ANALOG_GAIN_4 <= gain_val) && (gain_val < ANALOG_GAIN_5)) {

		sensor_write(sd, 0xb6, 0x03);
		tmp = 64 * gain_val / ANALOG_GAIN_4;
		sensor_write(sd, 0xb1, tmp >> 6);
		sensor_write(sd, 0xb2, (tmp << 2) & 0xfc);
	} else if ((ANALOG_GAIN_5 <= gain_val) && (gain_val < ANALOG_GAIN_6)) {

		sensor_write(sd, 0xb6, 0x04);
		tmp = 64 * gain_val / ANALOG_GAIN_5;
		sensor_write(sd, 0xb1, tmp >> 6);
		sensor_write(sd, 0xb2, (tmp << 2) & 0xfc);
	} else if ((ANALOG_GAIN_6 <= gain_val) && (gain_val < ANALOG_GAIN_7)) {

		sensor_write(sd, 0xb6, 0x05);
		tmp = 64 * gain_val / ANALOG_GAIN_6;
		sensor_write(sd, 0xb1, tmp >> 6);
		sensor_write(sd, 0xb2, (tmp << 2) & 0xfc);
	} else if ((ANALOG_GAIN_7 <= gain_val) && (gain_val < ANALOG_GAIN_8)) {

		sensor_write(sd, 0xb6, 0x06);
		tmp = 64 * gain_val / ANALOG_GAIN_7;
		sensor_write(sd, 0xb1, tmp >> 6);
		sensor_write(sd, 0xb2, (tmp << 2) & 0xfc);
	} else if ((ANALOG_GAIN_8 <= gain_val) && (gain_val < ANALOG_GAIN_9)) {

		sensor_write(sd, 0xb6, 0x07);
		tmp = 64 * gain_val / ANALOG_GAIN_8;
		sensor_write(sd, 0xb1, tmp >> 6);
		sensor_write(sd, 0xb2, (tmp << 2) & 0xfc);
	} else if ((ANALOG_GAIN_9 <= gain_val) && (gain_val < ANALOG_GAIN_10)) {

		sensor_write(sd, 0xb6, 0x08);
		tmp = 64 * gain_val / ANALOG_GAIN_9;
		sensor_write(sd, 0xb1, tmp >> 6);
		sensor_write(sd, 0xb2, (tmp << 2) & 0xfc);
	} else if ((ANALOG_GAIN_10 <= gain_val) && (gain_val < ANALOG_GAIN_11)) {

		sensor_write(sd, 0xb6, 0x09);
		tmp = 64 * gain_val / ANALOG_GAIN_10;
		sensor_write(sd, 0xb1, tmp >> 6);
		sensor_write(sd, 0xb2, (tmp << 2) & 0xfc);
	} else if (ANALOG_GAIN_11 <= gain_val) {

		sensor_write(sd, 0xb6, 0x0a);
		tmp = 64 * gain_val / ANALOG_GAIN_11;
		sensor_write(sd, 0xb1, tmp >> 6);
		sensor_write(sd, 0xb2, (tmp << 2) & 0xfc);
	}
	info->gain = gain_val;

	printk("gc1054 sensor_set_gain = %d, %d (1,2,4,8,15->0,1,2,3,4) Done!\n", gain_val, gain_val/16/6);

	return 0;
}

static int sensor_s_exp_gain(struct v4l2_subdev *sd, struct sensor_exp_gain *exp_gain)
{
	int exp_val, gain_val;
	struct sensor_info *info = to_state(sd);

	exp_val = exp_gain->exp_val;
	gain_val = exp_gain->gain_val;

	if (gain_val < 1 * 16)
		gain_val = 16;

	/* printk("------------%s  %d !!!!!!!\n", __func__, exp_val); */

	sensor_s_exp(sd, exp_val);
	sensor_s_gain(sd, gain_val);

	info->exp = exp_val;
	info->gain = gain_val;
	return 0;
}

/*
 *set && get sensor flip
 */

static int sensor_get_fmt_mbus_core(struct v4l2_subdev *sd, int *code)
{
	struct sensor_info *info = to_state(sd);
	data_type get_value;

	switch (flip_status) {
	case 0xc0:
		*code = MEDIA_BUS_FMT_SRGGB10_1X10;
		break;
	case 0xc1:
		*code = MEDIA_BUS_FMT_SGRBG10_1X10;
		break;
	case 0xc2:
		*code = MEDIA_BUS_FMT_SGBRG10_1X10;
		break;
	case 0xc3:
		*code = MEDIA_BUS_FMT_SBGGR10_1X10;
		break;
	default:
		*code = info->fmt->mbus_code;
	}
	return 0;
}

static int sensor_s_hflip(struct v4l2_subdev *sd, int enable)
{
	return 0;
}

static int sensor_s_vflip(struct v4l2_subdev *sd, int enable)
{
	return 0;
}



/*
 * Stuff that knows about the sensor.
 */
static int sensor_power(struct v4l2_subdev *sd, int on)
{
	int ret = 0;

	switch (on) {
	case STBY_ON:
		DBG_INFO("STBY_ON!\n");
		cci_lock(sd);
		vin_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		cci_unlock(sd);
		break;
	case STBY_OFF:
		DBG_INFO("STBY_OFF!\n");
		cci_lock(sd);
		vin_set_mclk_freq(sd, MCLK);
		vin_set_mclk(sd, ON);
		usleep_range(10000, 12000);
		vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		usleep_range(10000, 12000);
		cci_unlock(sd);
		usleep_range(10000, 12000);
		break;
	case PWR_ON:
		DBG_INFO("PWR_ON!\n");
		cci_lock(sd);
		vin_gpio_set_status(sd, PWDN, 1);
		vin_gpio_set_status(sd, RESET, 1);
		vin_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		usleep_range(1000, 1200);
		vin_gpio_write(sd, POWER_EN, CSI_GPIO_HIGH);
		vin_set_pmu_channel(sd, IOVDD, ON);
		usleep_range(100, 120);
		vin_set_pmu_channel(sd, DVDD, ON);
		usleep_range(1000, 1200);
		vin_set_pmu_channel(sd, AVDD, ON);
		vin_set_mclk(sd, ON);
		usleep_range(1000, 1200);
		vin_set_mclk_freq(sd, MCLK);
		usleep_range(10000, 12000);
		vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		usleep_range(10000, 12000);
		vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		usleep_range(30000, 31000);
		cci_unlock(sd);
		break;
	case PWR_OFF:
		DBG_INFO("PWR_OFF!do nothing\n");
		break;
		cci_lock(sd);
		vin_set_mclk(sd, OFF);
		vin_set_pmu_channel(sd, AVDD, OFF);
		vin_set_pmu_channel(sd, DVDD, OFF);
		vin_set_pmu_channel(sd, IOVDD, OFF);
		usleep_range(10000, 12000);
		vin_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		vin_gpio_set_status(sd, RESET, 0);
		vin_gpio_set_status(sd, PWDN, 0);
		cci_unlock(sd);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sensor_reset(struct v4l2_subdev *sd, u32 val)
{

	DBG_INFO("%s: val=%d\n", __func__);
	switch (val) {
	case 0:
		vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		usleep_range(10000, 12000);
		break;
	case 1:
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		usleep_range(10000, 12000);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sensor_detect(struct v4l2_subdev *sd)
{
	data_type rdval;
	int eRet;
	int times_out = 3;
	do {
		eRet = sensor_read(sd, ID_REG_HIGH, &rdval);
		DBG_INFO("eRet:%d, ID_VAL_HIGH:0x%x, times_out:%d\n", eRet, rdval, times_out);
		usleep_range(200000, 220000);
		times_out--;
	} while (eRet < 0 && times_out > 0);

	sensor_read(sd, ID_REG_HIGH, &rdval);
	DBG_INFO("ID_VAL_HIGH = %2x, Done!\n", rdval);
	if (rdval != ID_VAL_HIGH)
		return -ENODEV;
	sensor_read(sd, ID_REG_LOW, &rdval);
	DBG_INFO("ID_VAL_LOW = %2x, Done!\n", rdval);
	DBG_INFO("chenweihong!!!!!!!!!!!!!!!!!!\n");
	if (rdval != ID_VAL_LOW)
		return -ENODEV;
	DBG_INFO("Done!\n");
	return 0;
}

static int sensor_init(struct v4l2_subdev *sd, u32 val)
{
	int ret;
	struct sensor_info *info = to_state(sd);

	sensor_dbg("sensor_init\n");

	/* Make sure it is a target sensor */
	ret = sensor_detect(sd);
	if (ret) {
		sensor_err("chip found is not an target chip.\n");
		return ret;
	}

	info->focus_status = 0;
	info->low_speed = 0;
	info->width = 1280;
	info->height = 720;
	info->hflip = 0;
	info->vflip = 0;
	info->gain = 0;

	info->tpf.numerator = 1;
	info->tpf.denominator = 30;	/* 30fps */
	info->preview_first_flag = 1;
	return 0;
}

static long sensor_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	int ret = 0;
	struct sensor_info *info = to_state(sd);

	switch (cmd) {
	case GET_CURRENT_WIN_CFG:
		sensor_dbg("%s: GET_CURRENT_WIN_CFG, info->current_wins=%p\n", __func__, info->current_wins);

		if (info->current_wins != NULL) {
			memcpy(arg, info->current_wins,
				sizeof(*info->current_wins));
			ret = 0;
		} else {
			sensor_err("empty wins!\n");
			ret = -1;
		}
		break;
	case SET_FPS:
		break;
	case VIDIOC_VIN_SENSOR_EXP_GAIN:
		sensor_s_exp_gain(sd, (struct sensor_exp_gain *)arg);
		break;
	case VIDIOC_VIN_SENSOR_CFG_REQ:
		sensor_cfg_req(sd, (struct sensor_config *)arg);
		break;
	case VIDIOC_VIN_GET_SENSOR_CODE:
		sensor_get_fmt_mbus_core(sd, (int *)arg);
		break;
	default:
		return -EINVAL;
	}
	return ret;
}

/*
 * Store information about the video data format.
 */
static struct sensor_format_struct sensor_formats[] = {
	{
		.desc = "Raw RGB Bayer",
		.mbus_code = MEDIA_BUS_FMT_SBGGR10_1X10,/* .mbus_code = MEDIA_BUS_FMT_SBGGR10_1X10, */
		.regs = sensor_fmt_raw,
		.regs_size = ARRAY_SIZE(sensor_fmt_raw),
		.bpp = 1
	},
};
#define N_FMTS ARRAY_SIZE(sensor_formats)

/*
 * Then there is the issue of window sizes.  Try to capture the info here.
 */

static struct sensor_win_size sensor_win_sizes[] = {
	{
	.width = 1280,/* QSXGA_WIDTH, */
	.height = 720,/* QSXGA_HEIGHT, */
	.hoffset    = 0,
	.voffset    = 0,
	.hts        = 3200,
	.vts        = 1125,
	.pclk       = 72*1000*1000,
	.mipi_bps   = 312*1000*1000,
	.fps_fixed  = 20,
	.bin_factor = 1,
	.intg_min   = 1<<4,
	.intg_max = (1125) << 4,
	.gain_min   = 1<<4,
	.gain_max   = 32<<4,
	.regs = sensor_720p20_regs,
	.regs_size = ARRAY_SIZE(sensor_720p20_regs),
	.set_size = NULL,
	},
};

#define N_WIN_SIZES (ARRAY_SIZE(sensor_win_sizes))

static int sensor_g_mbus_config(struct v4l2_subdev *sd,
				struct v4l2_mbus_config *cfg)
{
	struct sensor_info *info = to_state(sd);

	cfg->type = V4L2_MBUS_CSI2_DPHY;
	cfg->flags = 0 | V4L2_MBUS_CSI2_1_LANE | V4L2_MBUS_CSI2_CHANNEL_0;

	return 0;
}

static int sensor_g_ctrl(struct v4l2_ctrl *ctrl)
{
	struct sensor_info *info =
			container_of(ctrl->handler, struct sensor_info, handler);
	struct v4l2_subdev *sd = &info->sd;

	switch (ctrl->id) {
	case V4L2_CID_GAIN:
		return sensor_g_gain(sd, &ctrl->val);
	case V4L2_CID_EXPOSURE:
		return sensor_g_exp(sd, &ctrl->val);
	}
	return -EINVAL;
}

static int sensor_s_ctrl(struct v4l2_ctrl *ctrl)
{
/*
	struct v4l2_queryctrl qc;
	int ret;
*/
	struct sensor_info *info =
			container_of(ctrl->handler, struct sensor_info, handler);
	struct v4l2_subdev *sd = &info->sd;
/*
	qc.id = ctrl->id;
	ret = sensor_queryctrl(sd, &qc);

	if (ret < 0)
		return ret;
	if (ctrl->val < qc.minimum || ctrl->val > qc.maximum)
		return -ERANGE;
*/

	switch (ctrl->id) {
	case V4L2_CID_GAIN:
		return sensor_s_gain(sd, ctrl->val);
	case V4L2_CID_EXPOSURE:
		return sensor_s_exp(sd, ctrl->val);
	case V4L2_CID_HFLIP:
		return sensor_s_hflip(sd, ctrl->val);
	case V4L2_CID_VFLIP:
		return sensor_s_vflip(sd, ctrl->val);
	}
	return -EINVAL;
}

static int sensor_reg_init(struct sensor_info *info)
{
	int ret;
	data_type rdval_l, rdval_h;
	struct v4l2_subdev *sd = &info->sd;
	struct sensor_format_struct *sensor_fmt = info->fmt;
	struct sensor_win_size *wsize = info->current_wins;

	sensor_dbg("sensor_reg_init, ARRAY_SIZE(sensor_default_regs)=%d\n", ARRAY_SIZE(sensor_default_regs));

	ret = sensor_write_array(sd, sensor_default_regs,
				 ARRAY_SIZE(sensor_default_regs));
	if (ret < 0) {
		sensor_err("write sensor_default_regs error\n");
		return ret;
	}

	sensor_dbg("sensor_reg_init, wsize=%p, wsize->regs=0x%x, wsize->regs_size=%d\n", wsize, wsize->regs, wsize->regs_size);

	sensor_write_array(sd, sensor_fmt->regs, sensor_fmt->regs_size);

	if (wsize->regs) {
		sensor_dbg("%s: start sensor_write_array(wsize->regs)\n", __func__);
		sensor_write_array(sd, wsize->regs, wsize->regs_size);
	}

	if (wsize->set_size)
		wsize->set_size(sd);

	info->width = wsize->width;
	info->height = wsize->height;
	flip_status = 0xc3;

	sensor_dbg("s_fmt set width = %d, height = %d\n", wsize->width,
		     wsize->height);

	return 0;
}

static int sensor_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct sensor_info *info = to_state(sd);

	sensor_dbg("%s on = %d, %d*%d fps: %d code: %x\n", __func__, enable,
		     info->current_wins->width, info->current_wins->height,
		     info->current_wins->fps_fixed, info->fmt->mbus_code);

	if (!enable)
		return 0;

	return sensor_reg_init(info);
}

/* ----------------------------------------------------------------------- */
static const struct v4l2_ctrl_ops sensor_ctrl_ops = {
	.g_volatile_ctrl = sensor_g_ctrl,
	.s_ctrl = sensor_s_ctrl,
	/* .queryctrl = sensor_queryctrl, */
};

static const struct v4l2_subdev_core_ops sensor_core_ops = {

	.reset = sensor_reset,
	.init = sensor_init,
	.s_power = sensor_power,
	.ioctl = sensor_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl32 = sensor_compat_ioctl32,
#endif
};

static const struct v4l2_subdev_video_ops sensor_video_ops = {
	.s_stream = sensor_s_stream,
	.g_mbus_config = sensor_g_mbus_config,
};

static const struct v4l2_subdev_pad_ops sensor_pad_ops = {
	.enum_mbus_code = sensor_enum_mbus_code,
	.enum_frame_size = sensor_enum_frame_size,
	.get_fmt = sensor_get_fmt,
	.set_fmt = sensor_set_fmt,
};

static const struct v4l2_subdev_ops sensor_ops = {
	.core = &sensor_core_ops,
	.video = &sensor_video_ops,
	.pad = &sensor_pad_ops,
};

/* ----------------------------------------------------------------------- */
static struct cci_driver cci_drv[] = {
	{
		.name = SENSOR_NAME,
		.addr_width = CCI_BITS_8,
		.data_width = CCI_BITS_8,
	}, {
		.name = SENSOR_NAME_2,
		.addr_width = CCI_BITS_8,
		.data_width = CCI_BITS_8,
	}
};

static int sensor_init_controls(struct v4l2_subdev *sd, const struct v4l2_ctrl_ops *ops)
{
	struct sensor_info *info = to_state(sd);
	struct v4l2_ctrl_handler *handler = &info->handler;
	struct v4l2_ctrl *ctrl;
	int ret = 0;

	v4l2_ctrl_handler_init(handler, 4);

	ctrl = v4l2_ctrl_new_std(handler, ops, V4L2_CID_GAIN, 1 * 1600,
			      256 * 1600, 1, 1 * 1600);

	if (ctrl != NULL)
		ctrl->flags |= V4L2_CTRL_FLAG_VOLATILE;

	ctrl = v4l2_ctrl_new_std(handler, ops, V4L2_CID_EXPOSURE, 1,
			      65536 * 16, 1, 1);
	v4l2_ctrl_new_std(handler, ops, V4L2_CID_HFLIP, 0, 1, 1, 0);
	v4l2_ctrl_new_std(handler, ops, V4L2_CID_VFLIP, 0, 1, 1, 0);

	if (ctrl != NULL)
		ctrl->flags |= V4L2_CTRL_FLAG_VOLATILE;

	if (handler->error) {
		ret = handler->error;
		v4l2_ctrl_handler_free(handler);
	}

	sd->ctrl_handler = handler;

	return ret;
}
static int sensor_dev_id;

static int sensor_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct sensor_info *info;
	int i;

	info = kzalloc(sizeof(*info), GFP_KERNEL);
	if (info == NULL)
		return -ENOMEM;
	sd = &info->sd;

	if (client) {
		for (i = 0; i < SENSOR_NUM; i++) {
			if (!strcmp(cci_drv[i].name, client->name))
				break;
		}
		cci_dev_probe_helper(sd, client, &sensor_ops, &cci_drv[i]);
	} else {
		cci_dev_probe_helper(sd, client, &sensor_ops, &cci_drv[sensor_dev_id++]);
	}

	sensor_init_controls(sd, &sensor_ctrl_ops);

	mutex_init(&info->lock);

	info->fmt = &sensor_formats[0];
	info->fmt_pt = &sensor_formats[0];
	info->win_pt = &sensor_win_sizes[0];
	info->fmt_num = N_FMTS;
	info->win_size_num = N_WIN_SIZES;
	info->sensor_field = V4L2_FIELD_NONE;
	/* use CMB_PHYA_OFFSET2  also ok */
	info->combo_mode = CMB_TERMINAL_RES | CMB_PHYA_OFFSET3 | MIPI_NORMAL_MODE;
	/* info->combo_mode = CMB_PHYA_OFFSET2 | MIPI_NORMAL_MODE; */
	info->stream_seq = MIPI_BEFORE_SENSOR;
	info->af_first_flag = 1;
	info->exp = 0;
	info->gain = 0;

	return 0;
}

static int sensor_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd;
	int i;

	if (client) {
		for (i = 0; i < SENSOR_NUM; i++) {
			if (!strcmp(cci_drv[i].name, client->name))
				break;
		}
		sd = cci_dev_remove_helper(client, &cci_drv[i]);
	} else {
		sd = cci_dev_remove_helper(client, &cci_drv[sensor_dev_id++]);
	}

	kfree(to_state(sd));
	return 0;
}

static const struct i2c_device_id sensor_id[] = {
	{SENSOR_NAME, 0},
	{}
};

static const struct i2c_device_id sensor_id_2[] = {
	{SENSOR_NAME_2, 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, sensor_id);
MODULE_DEVICE_TABLE(i2c, sensor_id_2);

static struct i2c_driver sensor_driver[] = {
	{
		.driver = {
			   .owner = THIS_MODULE,
			   .name = SENSOR_NAME,
			   },
		.probe = sensor_probe,
		.remove = sensor_remove,
		.id_table = sensor_id,
	}, {
		.driver = {
			   .owner = THIS_MODULE,
			   .name = SENSOR_NAME_2,
			   },
		.probe = sensor_probe,
		.remove = sensor_remove,
		.id_table = sensor_id_2,
	},
};
static __init int init_sensor(void)
{
	int i, ret = 0;

	sensor_dev_id = 0;

	for (i = 0; i < SENSOR_NUM; i++)
		ret = cci_dev_init_helper(&sensor_driver[i]);

	return ret;
}

static __exit void exit_sensor(void)
{
	int i;

	sensor_dev_id = 0;

	for (i = 0; i < SENSOR_NUM; i++)
		cci_dev_exit_helper(&sensor_driver[i]);
}

module_init(init_sensor);
module_exit(exit_sensor);
