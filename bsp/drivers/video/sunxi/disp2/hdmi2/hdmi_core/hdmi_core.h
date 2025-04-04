/*
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */


#ifndef PLATFORM_H_
#define PLATFORM_H_
#include "../config.h"

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/atomic.h>
#include <linux/kthread.h>
#include <linux/poll.h>
#include <linux/fcntl.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <video/sunxi_display2.h>
#include <video/drv_hdmi.h>

#include "core_edid.h"
#include "core_cec.h"
#include "api/core_api.h"
#include "api/general_ops.h"

enum hdcp_status {
	HDCP_DISABLE = 0,
	HDCP_ING = 1,
	HDCP_FAILED,
	HDCP_SUCCESS,
};

enum hpd_state_value {
	HDMI_State_Idle = 0,
	HDMI_State_Hdcp_Irq,
	HDMI_State_Wait_Hpd,
	HDMI_State_Rx_Sense,
	HDMI_State_EDID_Parse,
	HDMI_State_HPD_Done
};

enum irq_type {
	NONE_IRQ = 0,
	HDCP_IRQ = 1,
	HPD_IRQ
};

struct hdmi_mode {
	videoParams_t		pVideo;
	audioParams_t		pAudio;
	hdcpParams_t		pHdcp;
	productParams_t		pProduct;

	/* productParams_t	mProduct; */
	/* hdmivsdb_t		vsdb; */
	/* hdmiforumvsdb_t	forumvsdb; */

	/* uint8_t			ksv_list_buffer[670]; */
	uint8_t			ksv_devices;
	uint8_t			dpk_aksv[7];
	uint8_t			sw_enc_key[2];
	uint8_t			dpk_keys[560];

	int			edid_done;

	struct edid	      *edid;
	u8		      *edid_ext; /* edid extenssion raw data */
	sink_edid_t	      *sink_cap;
};


/* ***************video***************************** */
/*
#define HDMI1440_480I           6
#define HDMI1440_576I           21
#define HDMI480P                        2
#define HDMI576P                        17
#define HDMI720P_50                     19
#define HDMI720P_60             4
#define HDMI1080I_50            20
#define HDMI1080I_60            5
#define HDMI1080P_50            31
#define HDMI1080P_60            16
#define HDMI1080P_24            32
#define HDMI1080P_25            33
#define HDMI1080P_30            34
*/
enum HDMI_VIC {
	HDMI_VIC_640x480P60 = 1,
	HDMI_VIC_720x480P60_4_3,
	HDMI_VIC_720x480P60_16_9,
	HDMI_VIC_1280x720P60,
	HDMI_VIC_1920x1080I60,
	HDMI_VIC_720x480I_4_3,
	HDMI_VIC_720x480I_16_9,
	HDMI_VIC_720x240P_4_3,
	HDMI_VIC_720x240P_16_9,
	HDMI_VIC_1920x1080P60 = 16,
	HDMI_VIC_720x576P_4_3,
	HDMI_VIC_720x576P_16_9,
	HDMI_VIC_1280x720P50,
	HDMI_VIC_1920x1080I50,
	HDMI_VIC_720x576I_4_3,
	HDMI_VIC_720x576I_16_9,
	HDMI_VIC_1920x1080P50 = 31,
	HDMI_VIC_1920x1080P24,
	HDMI_VIC_1920x1080P25,
	HDMI_VIC_1920x1080P30,
	HDMI_VIC_1280x720P24 = 60,
	HDMI_VIC_1280x720P25,
	HDMI_VIC_1280x720P30,
	HDMI_VIC_3840x2160P24 = 93,
	HDMI_VIC_3840x2160P25,
	HDMI_VIC_3840x2160P30,
	HDMI_VIC_3840x2160P50,
	HDMI_VIC_3840x2160P60,
	HDMI_VIC_4096x2160P24,
	HDMI_VIC_4096x2160P25,
	HDMI_VIC_4096x2160P30,
	HDMI_VIC_4096x2160P50,
	HDMI_VIC_4096x2160P60,

	HDMI_VIC_2560x1440P60 = 0x201,
	HDMI_VIC_1440x2560P70 = 0x202,
	HDMI_VIC_1080x1920P60 = 0x203,
};

#define HDMI_RANGE 0X100000 /* use for HDMI address range out of range detection */

#define HDMI1080P_24_3D_FP  (HDMI_VIC_1920x1080P24 + 0x80)
#define HDMI720P_50_3D_FP   (HDMI_VIC_1280x720P50 + 0x80)
#define HDMI720P_60_3D_FP   (HDMI_VIC_1280x720P60 + 0x80)
/*
#define HDMI3840_2160P_30   95
#define HDMI3840_2160P_25   94
#define HDMI3840_2160P_24   93
#define HDMI4096_2160P_24   98
#define HDMI4096_2160P_25   99
#define HDMI4096_2160P_30   100

#define HDMI3840_2160P_50   96
#define HDMI3840_2160P_60   97
#define HDMI4096_2160P_50   101
#define HDMI4096_2160P_60   102
*/
struct disp_hdmi_mode {
	enum disp_tv_mode mode;
	int hdmi_mode;/* vic */
};

struct blacklist_sink {
	u8  mft_id[2];/* EDID manufacture id */
	u8  stib[13];/* EDID standard timing information blocks */
	u8  checksum;
};

struct blacklist_issue {
	u32 tv_mode;
	u32 issue_type;
};

struct hdmi_sink_blacklist {
	struct blacklist_sink sink;
	struct blacklist_issue issue[10];
};

struct api_function {
	int (*phy_write)(hdmi_tx_dev_t *dev, u8 addr, u16 data);
	int (*phy_read)(hdmi_tx_dev_t *dev, u8 addr, u16 *value);

	int (*scdc_write)(hdmi_tx_dev_t *dev, u8 address, u8 size, u8 *data);
	int (*scdc_read)(hdmi_tx_dev_t *dev, u8 address, u8 size, u8 *data);
};
/**
 * Main structure
 */
struct hdmi_tx_core {
	int				is_cts;
	enum hpd_state_value            hpd_state;
	int                             blacklist_sink;
	u32								cec_super_standby;
	/* * PHY version */
	int                             hdmi_tx_phy;

	/* Reserved for API internal use only */
	/* * HDMI TX API Internals */
	struct device_access			dev_access;
	struct system_functions		    sys_functions;

	/* for hdmi api */
	hdmi_tx_dev_t		            hdmi_tx;

	/* * Application mode configurations */
	struct hdmi_mode			    mode;
	struct disp_device_config	    config;

	struct api_function			    api_func;

	struct hdmi_dev_func            dev_func;
};


extern u32 hdmi_hpd_mask;
extern u32 hdmi_clk_enable_mask;

extern u32 get_drv_hpd_state(void);

void hdmitx_write(uintptr_t addr, u32 data);
u32 hdmitx_read(uintptr_t addr);

void core_init_audio(struct hdmi_mode *cfg);


int hdmi_tx_core_init(struct hdmi_tx_core *core,
					int phy,
					videoParams_t *Video,
					audioParams_t *audio,
					hdcpParams_t  *hdcp);
void hdmi_core_exit(struct hdmi_tx_core *core);

void resistor_calibration_core(struct hdmi_tx_core *core, u32 reg, u32 data);
void hdmi_configure_core(struct hdmi_tx_core *core);


/* ***************************IRQ handler******************************* */
void hdmi_core_set_base_addr(uintptr_t reg_base);
uintptr_t hdmi_core_get_base_addr(void);

void set_platform(struct hdmi_tx_core *core);
struct hdmi_tx_core *get_platform(void);

u32 hdmi_core_get_hpd_state(void);


/* *******************video***************************** */
extern dtd_t *get_dtd(u8 code, u32 refreshRate);
void hdmi_hpd_out_core_process(struct hdmi_tx_core *core);
void hpd_sense_enbale_core(struct hdmi_tx_core *core);
int hdmi_mode_blacklist_check(u8 *sink_edid);
void hdmi_reconfig_format_by_blacklist(struct disp_device_config *config);
s32 set_static_config(struct disp_device_config *config);
s32 get_static_config(struct disp_device_config *config);
s32 set_dynamic_config(struct disp_device_dynamic_config *config);
s32 get_dynamic_config(struct disp_device_dynamic_config *config);

s32 hdmi_set_display_mode(u32 mode);
s32 hdmi_mode_support(u32 mode);
s32 hdmi_get_HPD_status(void);
s32 hdmi_core_get_csc_type(void);
s32 hdmi_core_get_color_range(void);
s32 hdmi_get_video_timming_info(struct disp_video_timings **video_info);
s32 hdmi_enable_core(void);
s32 hdmi_smooth_enable_core(void);
s32 hdmi_disable_core(void);

/* ******************* SND_HDMI for sunxi_v2 begain ************************** */
s32 snd_hdmi_audio_enable(u8 enable, u8 channel);
s32 snd_hdmi_set_audio_para(hdmi_audio_t *audio_para);
/* ******************* SND_HDMI for sunxi_v2 end ***************************** */

u32 hdmi_core_get_rxsense_state(void);
u32 hdmi_core_get_phy_pll_lock_state(void);
u32 hdmi_core_get_phy_power_state(void);
u32 hdmi_core_get_tmds_mode(void);
#ifndef SUPPORT_ONLY_HDMI14
u32 hdmi_core_get_scramble_state(void);
#endif
u32 hdmi_core_get_avmute_state(void);
u32 hdmi_core_get_color_depth(void);
u32 hdmi_core_get_pixelrepetion(void);
u32 hdmi_core_get_colorimetry(void);
u32 hdmi_core_get_pixel_format(void);
u32 hdmi_core_get_video_code(void);
u32 hdmi_core_get_audio_sample_freq(void);
u32 hdmi_core_get_audio_sample_size(void);
u32 hdmi_core_get_audio_layout(void);
u32 hdmi_core_get_audio_channel_count(void);
u32 hdmi_core_get_audio_n(void);

void hdmi_core_avmute_enable(u8 enable);
void hdmi_core_phy_power_enable(u8 enable);
void hdmi_core_dvimode_enable(u8 enable);

#endif /* PLATFORM_H_ */
