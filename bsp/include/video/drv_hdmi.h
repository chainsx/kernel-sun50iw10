/*
 * Allwinner SoCs hdmi driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __DRV_HDMI_H__
#define __DRV_HDMI_H__

typedef struct {
	__u8    hw_intf;        /* 0:iis  1:spdif 2:pcm */
	__u16	fs_between;     /* fs */
	__u32   sample_rate;    /* sample rate */
	__u8    clk_edge;       /* 0 */
	__u8    ch0_en;         /* 1 */
	__u8    ch1_en;         /* 0 */
	__u8	ch2_en;         /* 0 */
	__u8	ch3_en;         /* 0 */
	__u8	word_length;    /* 32 */
	__u8    shift_ctl;      /* 0 */
	__u8    dir_ctl;        /* 0 */
	__u8    ws_pol;
	__u8    just_pol;
	__u8    channel_num;
	__u8	data_raw;
	__u8    sample_bit;
	__u8    ca;     /* channel allocation */
} hdmi_audio_t;

typedef struct {
	__s32 (*hdmi_audio_enable)(__u8 mode, __u8 channel);
	__s32 (*hdmi_set_audio_para)(hdmi_audio_t *audio_para);
	__s32 (*hdmi_is_playback)(void);
} __audio_hdmi_func;

enum hdmi_hpd_status {
	STATUE_CLOSE = 0,
	STATUE_OPEN = 1,
};

/******************** SND_HDMI for sunxi_v2 begain ***************************/
#if IS_ENABLED(CONFIG_AW_HDMI_DISP2) || IS_ENABLED(CONFIG_AW_HDMI2_DISP2_SUNXI)
extern int snd_hdmi_get_func(__audio_hdmi_func *hdmi_func);
#else
static inline int snd_hdmi_get_func(__audio_hdmi_func *hdmi_func)
{
	pr_err("HDMI Audio API is disable\n");

	return -1;
}
#endif
/******************** SND_HDMI for sunxi_v2 end ******************************/
#endif
