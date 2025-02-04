/*
 * A V4L2 driver for nvp6134 cameras.
 *
 * Copyright (c) 2017 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 * Authors:  chenliang <michaelchen@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


/*
 *#include "type.h"
 *#include "../../para.h"
 *#include "../cci/cci.h"
 *#include "../csic_hw.h"
 *#include "include.h"
 *#include "common.h"
 *#include "nxp_6134/common.h"
 */
#include "type.h"
#include "video.h"

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/videodev2.h>
#include <media/v4l2-device.h>
#include <media/v4l2-mediabus.h>

#include "../camera.h"
#include "../sensor_helper.h"
#include "csi_dev_nvp6134.h"
#include <media/v4l2-subdev.h>
int chip_id[4];
int rev_id[4];
unsigned int nvp6134_iic_addr[4] = {0x60, 0x62, 0x64, 0x66};
unsigned int nvp6134_cnt;

/* #define COLORBAR_EN */

extern struct v4l2_subdev *gl_sd;
#define SENSOR_NAME "nvp6134"

u32 gpio_i2c_write(u8 da, u8 reg, u8 val)
{
	u32 ret;

	ret = cci_write_a8_d8(gl_sd, reg, val);
	/* u_twi_wr_8_8(reg, val, da>>1,3); */

	return ret;
}

u32 gpio_i2c_read(u8 da, u8 reg)
{
	u8 ret;
	u8 val = 0;

	ret = cci_read_a8_d8(gl_sd, reg, &val);
	/* ret = u_twi_rd_8_8(reg, &val, da>>1,3); */

	return val;
}

/*******************************************************************************
*	Description		: Get Device ID
*	Argurments		: dec(slave address)
*	Return value	: Device ID
*	Modify			:
*	warning			:
*******************************************************************************/
int check_id(unsigned int dec)
{
	int ret;

	gpio_i2c_write(dec, 0xFF, 0x00);
	ret = gpio_i2c_read(dec, 0xf4);
	return ret;
}

/*******************************************************************************
*	Description		: Get rev ID
*	Argurments		: dec(slave address)
*	Return value	: rev ID
*	Modify			:
*	warning			:
*******************************************************************************/
int check_rev(unsigned int dec)
{
	int ret;

	gpio_i2c_write(dec, 0xFF, 0x00);
	ret = gpio_i2c_read(dec, 0xf5);
	return ret;
}

/* For 0, 1, 5 */
void dump_bank(int bank)
{
	int i = 0;
	u32 ret = 0;
	gpio_i2c_write(nvp6134_iic_addr[0], 0xFF, bank);
	for (i = 0; i < 0xF6; i++) {

		if (i == 0 || i % 16 == 0)
			sensor_print("0x%02x-0x%02x: ", i, i + 15);
		ret = gpio_i2c_read(nvp6134_iic_addr[0], i);
		sensor_print("0x%02x ", ret);
		if ((i > 0) && ((i + 1) % 16) == 0)
			sensor_print("\n");
	}
}
EXPORT_SYMBOL(dump_bank);

void read_bank_value(void)
{

	u8 ret = 0;
	u8 i = 0;

	gpio_i2c_write(nvp6134_iic_addr[0], 0xFF, 0x00);
	for (i = 0; i < 0xF6; i++) {
		ret = gpio_i2c_read(nvp6134_iic_addr[0], i);
		sensor_print("Bank0[0x%2.2x] = 0x%2.2x\n", i, ret);
	}

	gpio_i2c_write(nvp6134_iic_addr[0], 0xFF, 0x01);
	for (i = 0; i < 0xF6; i++) {
		ret = gpio_i2c_read(nvp6134_iic_addr[0], i);
		sensor_print("Bank1[0x%2.2x] = 0x%2.2x\n", i, ret);
	}

	gpio_i2c_write(nvp6134_iic_addr[0], 0xFF, 0x05);
	for (i = 0; i < 0xF0; i++) {
		ret = gpio_i2c_read(nvp6134_iic_addr[0], i);
		sensor_print("Bank5[0x%2.2x] = 0x%2.2x\n", i, ret);
	}
}

int sensor_init_hardware(u32 val)
{
	/* int ret = 0; */
	int ch = 0;
	int chip = 0;

	/*
	 *check Device ID of maximum 4chip on the slave address,
	 * manage slave address. chip count.
	 */
	nvp6134_cnt = 1;
#ifdef AHD_1080P_1CH
	chip_id[0] = NVP6134B_R0_ID;
#else
	chip_id[0] = NVP6134_R0_ID;
#endif
	nvp6134_iic_addr[0] = 0x62;
/*
	for (chip = 0; chip < 1; chip++) {
		chip_id[chip] = check_id(nvp6134_iic_addr[chip]);
		rev_id[chip] = check_rev(nvp6134_iic_addr[chip]);
		if ((chip_id[chip] != NVP6134_R0_ID) &&
			(chip_id[chip] != NVP6134B_R0_ID)) {
			sensor_print("Device ID Error... %x\n", chip_id[chip]);
		} else {
			sensor_print("Device (0x%x) ID OK... %x\n",
				nvp6134_iic_addr[chip], chip_id[chip]);
			sensor_print("Device (0x%x) REV ... %x\n",
				nvp6134_iic_addr[chip], rev_id[chip]);
			nvp6134_iic_addr[nvp6134_cnt] = nvp6134_iic_addr[chip];
			if (nvp6134_cnt < chip)
				nvp6134_iic_addr[chip] = 0xFF;
			chip_id[nvp6134_cnt] = chip_id[chip];
			rev_id[nvp6134_cnt] = rev_id[chip];
			nvp6134_cnt++;
		}
	}
*/

	/* sensor_print("Chip Count = %d, [0x%x]\n", */
	/* nvp6134_cnt, nvp6134_iic_addr[0]); */

	/* initialize semaphore */
	/*	sema_init(&nvp6134_lock, 1); */

	/* initialize common value of AHD */
	for (chip = 0; chip < nvp6134_cnt; chip++)
		nvp6134_common_init(chip);

	/* set channel mode(AHD 1080P) each chip - default */
	for (ch = 0; ch < nvp6134_cnt * 4; ch++) {

		nvp6134_set_chnmode(ch, NTSC, NVP6134_VI_1080P_NOVIDEO);
#ifdef AHD_1080P_1CH
		nvp6134_set_chnmode(ch, PAL, NVP6134_VI_1080P_NOVIDEO);
		nvp6134_set_chnmode(ch, PAL, NVP6134_VI_1080P_2530);
#else
		nvp6134_set_chnmode(ch, PAL, NVP6134_VI_720P_2530);
#endif
	}

	/* set port(1MUX AHD 1080P) each chip - default */
	for (chip = 0; chip < nvp6134_cnt; chip++) {
		if (chip_id[chip] == NVP6134_R0_ID) {

			nvp6134_set_portmode(chip, 0,
				NVP6134_OUTMODE_4MUX_BT1120S, 0);

			/* 1 bt1120 output 1ch data */
			/* ;NVP6134_OUTMODE_2MUX_BT1120S:2ch */
			/* data;NVP6134_OUTMODE_4MUX_BT1120S:4ch data */

			/*
			nvp6134_set_portmode(chip, 1,
					     NVP6134_OUTMODE_4MUX_BT1120S, 1);
			nvp6134_set_portmode(chip, 2,
					     NVP6134_OUTMODE_4MUX_BT1120S, 2);
			nvp6134_set_portmode(chip, 3,
					NVP6134_OUTMODE_4MUX_BT1120S, 3);
			*/
		} else if (chip_id[chip] == NVP6134B_R0_ID) {

			/* nvp6134_set_portmode(chip, 1,
					     NVP6134_OUTMODE_4MUX_BT1120S, 1);
			nvp6134_set_portmode(chip, 2,
					     NVP6134_OUTMODE_4MUX_BT1120S, 0); */
			nvp6134_set_portmode(chip, 1,
					     NVP6134_OUTMODE_1MUX_FHD, 1);
			nvp6134_set_portmode(chip, 2,
					     NVP6134_OUTMODE_1MUX_FHD, 0);
		}
	}
#ifndef AHD_1080P_1CH
	gpio_i2c_write(nvp6134_iic_addr[0], 0xFF, 0x00);
	/* ch1 */
	gpio_i2c_write(nvp6134_iic_addr[0], 0x0c, 0x00);
	gpio_i2c_write(nvp6134_iic_addr[0], 0x10, 0x80);
	gpio_i2c_write(nvp6134_iic_addr[0], 0x3c, 0x80);
	gpio_i2c_write(nvp6134_iic_addr[0], 0x40, 0x00);
	gpio_i2c_write(nvp6134_iic_addr[0], 0x44, 0xf0);
	gpio_i2c_write(nvp6134_iic_addr[0], 0x48, 0x30);
	gpio_i2c_write(nvp6134_iic_addr[0], 0x4c, 0xf6);
	gpio_i2c_write(nvp6134_iic_addr[0], 0x50, 0xf0);
	/* ch2 */
	gpio_i2c_write(nvp6134_iic_addr[0], 0x0d, 0x00);
	gpio_i2c_write(nvp6134_iic_addr[0], 0x11, 0x80);
	gpio_i2c_write(nvp6134_iic_addr[0], 0x3d, 0x80);
	gpio_i2c_write(nvp6134_iic_addr[0], 0x41, 0x00);
	gpio_i2c_write(nvp6134_iic_addr[0], 0x45, 0xf0);
	gpio_i2c_write(nvp6134_iic_addr[0], 0x49, 0x30);
	gpio_i2c_write(nvp6134_iic_addr[0], 0x4d, 0xf6);
	gpio_i2c_write(nvp6134_iic_addr[0], 0x51, 0xf0);
	/* ch3 */
	gpio_i2c_write(nvp6134_iic_addr[0], 0x0e, 0x00);
	gpio_i2c_write(nvp6134_iic_addr[0], 0x12, 0x80);
	gpio_i2c_write(nvp6134_iic_addr[0], 0x3e, 0x80);
	gpio_i2c_write(nvp6134_iic_addr[0], 0x42, 0x00);
	gpio_i2c_write(nvp6134_iic_addr[0], 0x46, 0xf0);
	gpio_i2c_write(nvp6134_iic_addr[0], 0x4a, 0x30);
	gpio_i2c_write(nvp6134_iic_addr[0], 0x4e, 0xf6);
	gpio_i2c_write(nvp6134_iic_addr[0], 0x52, 0xf0);
	/* ch4 */
	gpio_i2c_write(nvp6134_iic_addr[0], 0x0f, 0x00);
	gpio_i2c_write(nvp6134_iic_addr[0], 0x13, 0x80);
	gpio_i2c_write(nvp6134_iic_addr[0], 0x3f, 0x80);
	gpio_i2c_write(nvp6134_iic_addr[0], 0x43, 0x00);
	gpio_i2c_write(nvp6134_iic_addr[0], 0x47, 0xf0);
	gpio_i2c_write(nvp6134_iic_addr[0], 0x4b, 0x30);
	gpio_i2c_write(nvp6134_iic_addr[0], 0x4f, 0xf6);
	gpio_i2c_write(nvp6134_iic_addr[0], 0x53, 0xf0);
#endif
#ifdef COLORBAR_EN
	gpio_i2c_write(nvp6134_iic_addr[0], 0xFF, 0x05);
	gpio_i2c_write(nvp6134_iic_addr[0], 0x2c, 0x08);
	gpio_i2c_write(nvp6134_iic_addr[0], 0xFF, 0x06);
	gpio_i2c_write(nvp6134_iic_addr[0], 0x2c, 0x08);
	gpio_i2c_write(nvp6134_iic_addr[0], 0xFF, 0x07);
	gpio_i2c_write(nvp6134_iic_addr[0], 0x2c, 0x08);
	gpio_i2c_write(nvp6134_iic_addr[0], 0xFF, 0x08);
	gpio_i2c_write(nvp6134_iic_addr[0], 0x2c, 0x08);

	gpio_i2c_write(nvp6134_iic_addr[0], 0xFF, 0x00);
	/* gpio_i2c_write(nvp6134_iic_addr[0], 0x78, 0x42);//ch1:Blue */
	/* ch2:Yellow ch3:Green ch4:Red */
	/* gpio_i2c_write(nvp6134_iic_addr[0], 0x79, 0x76); */
	gpio_i2c_write(nvp6134_iic_addr[0], 0x78,
		       0xce); /* ch1:Blue  ch2:Yellow ch3:Green ch4:Red */
	gpio_i2c_write(nvp6134_iic_addr[0], 0x79, 0xba);
#endif

	/* gpio_i2c_write(nvp6134_iic_addr[0], 0x7A, 0x44); */
	/* gpio_i2c_write(nvp6134_iic_addr[0], 0x7B, 0x44); */

	/* read_bank_value(3); */
	/* dump_bank(0); */
	/* dump_bank(1); */
	/* dump_bank(5); */

	/*	gpio_i2c_write(nvp6134_iic_addr[0], 0xFF, 0x01); */
	/*	gpio_i2c_write(nvp6134_iic_addr[0], 0xCA, 0x66); */

	/*	 initialize Audio */
	/*	 recmaster, pbmaster, ch_num, samplerate, bits */
	/*	audio_init(1,0,16,0,0); */

	/*	 create kernel thread for EQ, But Now not used. */
	/*	nvp6134_kt = kthread_create(nvp6134_kernel_thread, NULL, */
	/* "nvp6134_kt"); */
	/*    if(!IS_ERR(nvp6134_kt)) */
	/*        wake_up_process(nvp6134_kt); */
	/*    else { */
	/*        sensor_print("create nvp6134 watchdog thread failed!!\n"); */
	/*        nvp6134_kt = 0; */
	/*        return 0; */
	/*    } */

	return 0;
}
