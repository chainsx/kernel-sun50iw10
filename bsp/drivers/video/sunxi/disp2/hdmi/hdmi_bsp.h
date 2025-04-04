/*
 * Allwinner SoCs hdmi driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __HDMI_BSP_H_
#define __HDMI_BSP_H_

#include <linux/types.h>

#if defined(CONFIG_ARCH_SUN50IW2P1)
#define HDMI_USING_INNER_BIAS 1
#endif

#define LINUX_OS

#if defined(__LIB__)
#if defined(__ARM64__)
#define uintptr_t unsigned long
#else
#define uintptr_t unsigned int
#endif

#define __iomem
#endif
typedef struct {
	void (*delay_us)(unsigned long us);
	void (*delay_ms)(unsigned long ms);
} hdmi_bsp_func;

typedef struct {
	u8 mLength[8];
	u8 mBlock[64];
	int mIndex;
	int mComputed;
	int mCorrupted;
	unsigned mDigest[5];
} sha_t;

#define TRUE true
#define FALSE false

#ifndef NULL
#define NULL 0
#endif

#define BSTATUS_DEVICE_COUNT_MASK 0x007F
#define KSV_LEN  5
#define HEADER   10
#define SHAMAX   20

enum color_space {
	BT601 = 1,
	BT709,
	EXT_CSC,
};

struct video_para {
	unsigned int vic;
	enum color_space csc;
	unsigned char is_hdmi;
	unsigned char is_yuv;
	unsigned char is_hcts;
	unsigned int pixel_clk;
	unsigned int clk_div;
	unsigned int pixel_repeat;
	unsigned int x_res;
	unsigned int y_res;
	unsigned int hor_total_time;
	unsigned int hor_back_porch;
	unsigned int hor_front_porch;
	unsigned int hor_sync_time;
	unsigned int ver_total_time;
	unsigned int ver_back_porch;
	unsigned int ver_front_porch;
	unsigned int ver_sync_time;
	unsigned int hor_sync_polarity; /* 0: negative, 1: positive */
	unsigned int ver_sync_polarity; /* 0: negative, 1: positive */
	unsigned int b_interlace;
};

enum audio_type {
	PCM = 1,
	AC3,
	MPEG1,
	MP3,
	MPEG2,
	AAC,
	DTS,
	ATRAC,
	OBA,
	DDP,
	DTS_HD,
	MAT,
	DST,
	WMA_PRO,
};

struct audio_para {
	enum	audio_type	type;
	unsigned char			ca;
	unsigned int			sample_rate;
	unsigned int			sample_bit;
	unsigned int			ch_num;
	unsigned int			vic;
};

enum {
	HDMI_CEC_IMAGE_VIEW_ON                 = 0x04,
	HDMI_CEC_TEXT_VIEW_ON                  = 0x0d,
	HDMI_CEC_GIVE_DECK_STATUS              = 0x1a,
	HDMI_CEC_DECK_STATUS                   = 0x1b,
	HDMI_CEC_SET_MENU_LANGUAGE             = 0x32,
	HDMI_CEC_STANDBY                       = 0x36,
	HDMI_CEC_PLAY                          = 0x41,
	HDMI_CEC_DECK_CONTROL                  = 0x42,
	HDMI_CEC_SET_OSD_NAME                  = 0x47,
	HDMI_CEC_SENT_ROUTING_INFORMATION      = 0x81,
	HDMI_CEC_ACTIVE_SOURCE                 = 0x82,
	HDMI_CEC_REPORT_PHY_ADDR               = 0X84,
	HDMI_CEC_REQUEST_ACTIVE_SOURCE         = 0x85,
	HDMI_CEC_SENT_VERDOR_ID                = 0x87,
	HDMI_CEC_SENT_VERDOR_COMMAND           = 0x89,
	HDMI_CEC_MENU_STATE                    = 0x8e,
	HDMI_CEC_GIVE_POWER_STATUS             = 0x8f,
	HDMI_CEC_REPORT_POWER_STATUS           = 0x90,
	HDMI_CEC_GET_MENU_LANGUAGE             = 0x91,
	HDMI_CEC_INACTIVE_SOURCE               = 0x9d,
	HDMI_CEC_REPORT_CEC_VERSION            = 0x9e,
};

#if defined(CONFIG_ARCH_SUN8IW12) || defined(CONFIG_ARCH_SUN8IW7) || defined(CONFIG_ARCH_SUN8IW11)
#define HDMI_ENABLE_DUMP_WRITE
#endif /* endif CONFIG_ARCH_SUN8IW12 */

int bsp_hdmi_set_func(hdmi_bsp_func *func);
void bsp_hdmi_set_addr(uintptr_t base_addr);
void bsp_hdmi_init(void);
void bsp_hdmi_set_video_en(unsigned char enable);
int bsp_hdmi_video(struct video_para *video);
int bsp_hdcp_enable(u32 enable, struct video_para *video);
int bsp_hdcp_disconfig(void);
int bsp_hdmi_audio(struct audio_para *audio);
int bsp_hdmi_ddc_read(char cmd, char pointer, char offset,
			int nbyte, char *pbuf);

unsigned int bsp_hdmi_get_hpd(void);
void bsp_hdmi_standby(void);
void bsp_hdmi_hrst(void);
void bsp_hdmi_hdl(void);
/* @version: 0:A, 1:B, 2:C, 3:D */
void bsp_hdmi_set_version(unsigned int version);
int bsp_hdmi_hdcp_err_check(void);
int bsp_hdmi_cec_get_simple_msg(unsigned char *msg);
int bsp_hdmi_cec_send(char *buf, unsigned char bytes);
void bsp_hdmi_cec_free_time_set(unsigned char value);
int bsp_hdmi_cec_sta_check(void);
int bsp_hdmi_set_bias_source(unsigned int src);

#if defined(HDMI_ENABLE_DUMP_WRITE)
int bsp_hdmi_read(unsigned int addr, unsigned char *buf);
void bsp_hdmi_write(unsigned int addr, unsigned char data);
#endif

#endif
