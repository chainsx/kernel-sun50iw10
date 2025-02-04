/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include "de_rtmx.h"
#include "de_enhance.h"
#include "de_feat.h"

#define ONE_SCREEN_ONE_PARA	/* only ONE parameters for one screen, and all VEPs in this screen use SAME parameters */
/* DEBUG */
#define ASE_EXIST
#define LTI_EXIST
#define FCC_EXIST
#define BWS_EXIST

static int device_num;
static int vep_num[DEVICE_NUM];
static unsigned int frame_cnt[DEVICE_NUM] = { 0 };

static unsigned int g_demo_enable[DEVICE_NUM] = { 0 };

/* global histogram, use for bws and ce */
unsigned int *g_hist[DEVICE_NUM][CHN_NUM], *g_hist_p[DEVICE_NUM][CHN_NUM];
unsigned int g_sum[DEVICE_NUM][CHN_NUM];

extern __ce_status_t *g_ce_status[DEVICE_NUM][CHN_NUM];
extern __bws_status_t *g_bws_status[DEVICE_NUM][CHN_NUM];

#if 0				/* Do parameters and size/window apply 14-8-11 */
int de_enhance_info2data(struct disp_enhance_config *config,
			 vep_config_data *data)
{
	de_rect tmp_win;
	struct disp_enhance_config tmp_config;

	memcpy(&tmp_config.info, config, sizeof(struct disp_enhance_config));
	memset(data, 0, sizeof(vep_config_data));

	tmp_win.x = config->info.window.x;
	tmp_win.y = config->info.window.y;
	tmp_win.w = config->info.window.width;
	tmp_win.h = config->info.window.height;

	tmp_config.info.sharp =
	    (tmp_config.info.enable == 0) ? 0 : tmp_config.info.sharp;
	tmp_config.info.auto_color =
	    (tmp_config.info.enable == 0) ? 0 : tmp_config.info.auto_color;
	tmp_config.info.auto_contrast =
	    (tmp_config.info.enable == 0) ? 0 : tmp_config.info.auto_contrast;
	tmp_config.info.fancycolor_red =
	    (tmp_config.info.enable == 0) ? 0 : tmp_config.info.fancycolor_red;
	tmp_config.info.fancycolor_green =
	    (tmp_config.info.enable ==
	     0) ? 0 : tmp_config.info.fancycolor_green;
	tmp_config.info.fancycolor_blue =
	    (tmp_config.info.enable == 0) ? 0 : tmp_config.info.fancycolor_blue;

	/* fce */
	de_fce_info2para(tmp_config.info.sharp, tmp_config.info.auto_contrast,
			 tmp_config.info.auto_color, tmp_win, &data->fce_para);
#ifdef BWS_EXIST
	/* bws */
	de_bws_info2para(tmp_config.info.auto_contrast, tmp_win,
			 &data->bws_para);
#endif
	/* FIXME */
	/* peak */
	de_peak_info2para(tmp_config.info.sharp, tmp_win, &data->peak_para);
#ifdef LTI_EXIST
	/* lti */
	de_lti_info2para(tmp_config.info.sharp, tmp_win, &data->lti_para);
#endif
#ifdef ASE_EXIST
	/* ase */
	de_ase_info2para(tmp_config.info.auto_color, tmp_win, &data->ase_para);
#endif
#ifdef FCC_EXIST
	/* fcc */
	de_fcc_info2para(tmp_config.info.fancycolor_red,
			 tmp_config.info.fancycolor_green,
			 tmp_config.info.fancycolor_blue, 0, 0, 0, tmp_win,
			 &data->fcc_para);
#endif

	return 0;
}
#else
int de_enhance_info2data(struct disp_enhance_config *config,
			 vep_config_data *data)
{
	de_rect tmp_win;
	struct disp_enhance_config tmp_config;

	memcpy(&tmp_config.info, config, sizeof(struct disp_enhance_config));
	memset(data, 0, sizeof(vep_config_data));

	/* FIXME : should be deleted */
	tmp_win.x = config->info.window.x;
	tmp_win.y = config->info.window.y;
	tmp_win.w = config->info.window.width;
	tmp_win.h = config->info.window.height;
	/* FIXME end: should be deleted */

	tmp_config.info.sharp =
	    (tmp_config.info.enable == 0) ? 0 : tmp_config.info.sharp;
	tmp_config.info.auto_color =
	    (tmp_config.info.enable == 0) ? 0 : tmp_config.info.auto_color;
	tmp_config.info.auto_contrast =
	    (tmp_config.info.enable == 0) ? 0 : tmp_config.info.auto_contrast;
	tmp_config.info.fancycolor_red =
	    (tmp_config.info.enable == 0) ? 0 : tmp_config.info.fancycolor_red;
	tmp_config.info.fancycolor_green =
	    (tmp_config.info.enable ==
	     0) ? 0 : tmp_config.info.fancycolor_green;
	tmp_config.info.fancycolor_blue =
	    (tmp_config.info.enable == 0) ? 0 : tmp_config.info.fancycolor_blue;

	/* FIXME : tmp_win should be deleted */
	/* fce */
	de_fce_info2para(tmp_config.info.sharp, tmp_config.info.auto_contrast,
			 tmp_config.info.auto_color, tmp_win, &data->fce_para);
#ifdef BWS_EXIST
	/* bws */
	de_bws_info2para(tmp_config.info.auto_contrast, tmp_win,
			 &data->bws_para);
#endif
	/* FIXME */
	/* peak */
	de_peak_info2para(tmp_config.info.sharp, tmp_win, &data->peak_para);
#ifdef LTI_EXIST
	/* lti */
	de_lti_info2para(tmp_config.info.sharp, tmp_win, &data->lti_para);
#endif
#ifdef ASE_EXIST
	/* ase */
	de_ase_info2para(tmp_config.info.auto_color, tmp_win, &data->ase_para);
#endif
#ifdef FCC_EXIST
	/* fcc */
	de_fcc_info2para(tmp_config.info.fancycolor_red,
			 tmp_config.info.fancycolor_green,
			 tmp_config.info.fancycolor_blue, 0, 0, 0, tmp_win,
			 &data->fcc_para);
#endif

	return 0;
}

#endif

#if 0				/* Do parameters and size/window apply 14-8-11 */
int de_enhance_apply(unsigned int screen_id, struct disp_enhance_config *config)
{
	int ch_id, chno;
	vep_config_data data;
	int auto_contrast_dirty;

	chno = vep_num[screen_id];

	__inf("disp %d, en=%d, sharp=%d\n", screen_id, config[0].info.enable,
	      config[0].info.sharp);
	for (ch_id = 0; ch_id < chno; ch_id++) {
#ifdef ONE_SCREEN_ONE_PARA
		auto_contrast_dirty =
		    (config[0].flags & ENHANCE_ENABLE_DIRTY) ? 1 : 0;

		/* disp_enhance_info -> vep_config_data */
		de_enhance_info2data(&config[0], &data);
		/* FIXME: Update according to dirty flag */
		/* fce */
		de_hist_apply(screen_id, ch_id, data.fce_para.hist_en,
			      auto_contrast_dirty);
		de_ce_apply(screen_id, ch_id, data.fce_para.ce_en,
			    auto_contrast_dirty);
		de_fce_enable(screen_id, ch_id, data.fce_para.fce_en);
		de_fce_set_size(screen_id, ch_id, config[0].info.size.width,
				config[0].info.size.height);
		de_fce_set_para(screen_id, ch_id, data.fce_para.lce_en,
				data.fce_para.ftc_en, data.fce_para.ce_en,
				data.fce_para.hist_en);
		de_fce_set_window(screen_id, ch_id, data.fce_para.win_en,
				  data.fce_para.win);
		g_ce_status[screen_id][ch_id]->width =
		    config[0].info.size.width;
		g_ce_status[screen_id][ch_id]->height =
		    config[0].info.size.height;
#ifdef BWS_EXIST
		/* bws */
		de_bws_apply(screen_id, ch_id, data.bws_para.bws_en,
			     auto_contrast_dirty);
		de_bws_enable(screen_id, ch_id, data.bws_para.bws_en);
		de_bws_set_size(screen_id, ch_id, config[0].info.size.width,
				config[0].info.size.height);
		de_bws_set_window(screen_id, ch_id, data.bws_para.win_en,
				  data.bws_para.win);
		g_bws_status[screen_id][ch_id]->width =
		    config[0].info.size.width;
		g_bws_status[screen_id][ch_id]->height =
		    config[0].info.size.height;
#endif
		/* peak */
		de_peak_enable(screen_id, ch_id, data.peak_para.peak_en);
		de_peak_set_size(screen_id, ch_id, config[0].info.size.width,
				 config[0].info.size.height);
		de_peak_set_para(screen_id, ch_id, data.peak_para.gain);
		de_peak_set_window(screen_id, ch_id, data.peak_para.win_en,
				   data.peak_para.win);
#ifdef LTI_EXIST
		/* lti */
		de_lti_enable(screen_id, ch_id, data.lti_para.lti_en);
		de_lti_set_size(screen_id, ch_id, config[0].info.size.width,
				config[0].info.size.height);
		de_lti_set_para(screen_id, ch_id, data.lti_para.lti_en);
		de_lti_set_window(screen_id, ch_id, data.lti_para.win_en,
				  data.lti_para.win);
#endif
#ifdef ASE_EXIST
		/* ase */
		de_ase_enable(screen_id, ch_id, data.ase_para.ase_en);
		de_ase_set_size(screen_id, ch_id, config[0].info.size.width,
				config[0].info.size.height);
		de_ase_set_para(screen_id, ch_id, data.ase_para.gain);
		de_ase_set_window(screen_id, ch_id, data.ase_para.win_en,
				  data.ase_para.win);
#endif
#ifdef FCC_EXIST
		/* fcc */
		de_fcc_enable(screen_id, ch_id, data.fcc_para.fcc_en);
		de_fcc_set_size(screen_id, ch_id, config[0].info.size.width,
				config[0].info.size.height);
		de_fcc_set_para(screen_id, ch_id, data.fcc_para.sgain);
		de_fcc_set_window(screen_id, ch_id, data.fcc_para.win_en,
				  data.fcc_para.win);
#endif
#else
		auto_contrast_dirty =
		    (config[ch_id].flags & ENHANCE_ENABLE_DIRTY) ? 1 : 0;
		/* disp_enhance_info -> vep_config_data */
		de_enhance_info2data(&config[ch_id], &data);
		/* fce */
		de_hist_apply(screen_id, ch_id, data.fce_para.hist_en,
			      auto_contrast_dirty);
		de_ce_apply(screen_id, ch_id, data.fce_para.ce_en,
			    auto_contrast_dirty);
		de_fce_enable(screen_id, ch_id, data.fce_para.fce_en);
		de_fce_set_size(screen_id, ch_id, config[ch_id].info.size.width,
				config[ch_id].info.size.height);
		de_fce_set_para(screen_id, ch_id, data.fce_para.lce_en,
				data.fce_para.ftc_en, data.fce_para.ce_en,
				data.fce_para.hist_en);
		de_fce_set_window(screen_id, ch_id, data.fce_para.win_en,
				  data.fce_para.win);
		g_ce_status[screen_id][ch_id]->width =
		    config[ch_id].info.size.width;
		g_ce_status[screen_id][ch_id]->height =
		    config[ch_id].info.size.height;
		/* bws */
		de_bws_apply(screen_id, ch_id, data.bws_para.bws_en,
			     auto_contrast_dirty);
		de_bws_enable(screen_id, ch_id, data.bws_para.bws_en);
		de_bws_set_size(screen_id, ch_id, config[ch_id].info.size.width,
				config[ch_id].info.size.height);
		de_bws_set_window(screen_id, ch_id, data.bws_para.win_en,
				  data.bws_para.win);
		g_bws_status[screen_id][ch_id]->width =
		    config[ch_id].info.size.width;
		g_bws_status[screen_id][ch_id]->height =
		    config[ch_id].info.size.height;
		/* peak */
		de_peak_enable(screen_id, ch_id, data.peak_para.peak_en);
		de_peak_set_size(screen_id, ch_id,
				 config[ch_id].info.size.width,
				 config[ch_id].info.size.height);
		de_peak_set_para(screen_id, ch_id, data.peak_para.gain);
		de_peak_set_window(screen_id, ch_id, data.peak_para.win_en,
				   data.peak_para.win);
		/* lti */
		de_lti_enable(screen_id, ch_id, data.lti_para.gain);
		de_lti_set_size(screen_id, ch_id, config[ch_id].info.size.width,
				config[ch_id].info.size.height);
		de_lti_set_para(screen_id, ch_id, data.lti_para.lti_en);
		de_lti_set_window(screen_id, ch_id, data.lti_para.win_en,
				  data.lti_para.win);
		/* ase */
		de_ase_enable(screen_id, ch_id, data.ase_para.gain);
		de_ase_set_size(screen_id, ch_id, config[ch_id].info.size.width,
				config[ch_id].info.size.height);
		de_ase_set_para(screen_id, ch_id, data.ase_para.gain);
		de_ase_set_window(screen_id, ch_id, data.ase_para.win_en,
				  data.ase_para.win);
		/* fcc */
		de_fcc_enable(screen_id, ch_id, data.fcc_para.fcc_en);
		de_fcc_set_size(screen_id, ch_id, config[ch_id].info.size.width,
				config[ch_id].info.size.height);
		de_fcc_set_para(screen_id, ch_id, data.fcc_para.sgain);
		de_fcc_set_window(screen_id, ch_id, data.fcc_para.win_en,
				  data.fcc_para.win);

#endif
	}

	return 0;
}
#else
int de_enhance_apply(unsigned int screen_id, struct disp_enhance_config *config)
{
	int ch_id, chno;
	vep_config_data data;
	int auto_contrast_dirty;

	chno = vep_num[screen_id];

	__inf("disp %d, en=%d, sharp=%d\n", screen_id, config[0].info.enable,
	      config[0].info.sharp);
	/* de_enhance_set_mode(config); */
	for (ch_id = 0; ch_id < chno; ch_id++) {
		auto_contrast_dirty =
		    (config[0].flags & ENH_ENABLE_DIRTY) ? 1 : 0;

		/* disp_enhance_info -> vep_config_data */
		de_enhance_info2data(&config[0], &data);
		/* FIXME: Update according to dirty flag */
		/* fce */
		de_hist_apply(screen_id, ch_id, data.fce_para.hist_en,
			      auto_contrast_dirty);
		de_ce_apply(screen_id, ch_id, data.fce_para.ce_en,
			    auto_contrast_dirty);
		de_fce_enable(screen_id, ch_id, data.fce_para.fce_en);
		de_fce_set_para(screen_id, ch_id, data.fce_para.lce_en,
				data.fce_para.ftc_en, data.fce_para.ce_en,
				data.fce_para.hist_en);

		/* bws */
		de_bws_apply(screen_id, ch_id, data.bws_para.bws_en,
			     auto_contrast_dirty);
		de_bws_enable(screen_id, ch_id, data.bws_para.bws_en);

		/* peak */
		de_peak_enable(screen_id, ch_id, data.peak_para.peak_en);
		de_peak_set_para(screen_id, ch_id, data.peak_para.gain);

		/* lti */
		de_lti_enable(screen_id, ch_id, data.lti_para.lti_en);
		de_lti_set_para(screen_id, ch_id, data.lti_para.lti_en);

		/* ase */
		de_ase_enable(screen_id, ch_id, data.ase_para.ase_en);
		de_ase_set_para(screen_id, ch_id, data.ase_para.gain);

		/* fcc */
		de_fcc_enable(screen_id, ch_id, data.fcc_para.fcc_en);
		de_fcc_set_para(screen_id, ch_id, data.fcc_para.sgain);

	}
	de_enhance_demo_enable(screen_id, config[0].info.demo_enable);

	return 0;
}
#endif

int de_enhance_set_size(unsigned int screen_id, struct disp_rectsz *size)
{
	int ch_id, chno;
	de_rect tmp_win;
	unsigned int demo_enable;

	chno = vep_num[screen_id];
	demo_enable = g_demo_enable[screen_id] ? 1 : 0;

	for (ch_id = 0; ch_id < chno; ch_id++) {
		tmp_win.x = 0;
		tmp_win.y = 0;
		if (demo_enable) {
			if (size[ch_id].width > size[ch_id].height) {
				tmp_win.w = size[ch_id].width >> 1;
				tmp_win.h = size[ch_id].height;
			} else {
				tmp_win.w = size[ch_id].width;
				tmp_win.h = size[ch_id].height >> 1;
			}
		} else {
			tmp_win.w = size[ch_id].width;
			tmp_win.h = size[ch_id].height;
		}

		/* fce */
		de_fce_set_size(screen_id, ch_id, size[ch_id].width,
				size[ch_id].height);
		de_fce_set_window(screen_id, ch_id, demo_enable, tmp_win);
		g_ce_status[screen_id][ch_id]->width = size[ch_id].width;
		g_ce_status[screen_id][ch_id]->height = size[ch_id].height;

		/* bws */
		de_bws_set_size(screen_id, ch_id, size[ch_id].width,
				size[ch_id].height);
		de_bws_set_window(screen_id, ch_id, demo_enable, tmp_win);
		g_bws_status[screen_id][ch_id]->width = size[ch_id].width;
		g_bws_status[screen_id][ch_id]->height = size[ch_id].height;

		/* peak */
		de_peak_set_size(screen_id, ch_id, size[ch_id].width,
				 size[ch_id].height);
		de_peak_set_window(screen_id, ch_id, demo_enable, tmp_win);

		/* lti */
		de_lti_set_size(screen_id, ch_id, size[ch_id].width,
				size[ch_id].height);
		de_lti_set_window(screen_id, ch_id, demo_enable, tmp_win);

		/* ase */
		de_ase_set_size(screen_id, ch_id, size[ch_id].width,
				size[ch_id].height);
		de_ase_set_window(screen_id, ch_id, demo_enable, tmp_win);

		/* fcc */
		de_fcc_set_size(screen_id, ch_id, size[ch_id].width,
				size[ch_id].height);
		de_fcc_set_window(screen_id, ch_id, demo_enable, tmp_win);
	}

	return 0;
}

int de_enhance_demo_enable(unsigned int screen_id, unsigned int enable)
{
	if (enable) {
		g_demo_enable[screen_id] = 1;
	} else {
		g_demo_enable[screen_id] = 0;
	}
	return 0;
}

int de_enhance_sync(unsigned int screen_id)
{
	return 0;
}

int de_enhance_tasklet(unsigned int screen_id)
{
	int ch_id, chno;

	chno = vep_num[screen_id];

	for (ch_id = 0; ch_id < chno; ch_id++) {
		/* hist */
		de_hist_tasklet(screen_id, ch_id, frame_cnt[screen_id]);

		/* ce */
		de_ce_tasklet(screen_id, ch_id, frame_cnt[screen_id]);
#ifdef BWS_EXIST
		/* bws */
		de_bws_tasklet(screen_id, ch_id, frame_cnt[screen_id]);
#endif
		frame_cnt[screen_id]++;
	}
	return 0;
}

int de_enhance_update_regs(unsigned int screen_id)
{
	int chno, ch_id;

	chno = vep_num[screen_id];
	for (ch_id = 0; ch_id < chno; ch_id++) {
		de_fce_update_regs(screen_id, ch_id);
#ifdef BWS_EXIST
		de_bws_update_regs(screen_id, ch_id);
#endif
#ifdef LTI_EXIST
		de_lti_update_regs(screen_id, ch_id);
#endif
		de_peak_update_regs(screen_id, ch_id);
#ifdef ASE_EXIST
		de_ase_update_regs(screen_id, ch_id);
#endif
#ifdef FCC_EXIST
		de_fcc_update_regs(screen_id, ch_id);
#endif
	}
	return 0;
}

int de_enhance_init(struct disp_bsp_init_para *para)
{
	int screen_id, ch_id;

	device_num = de_feat_get_num_devices();

	for (screen_id = 0; screen_id < device_num; screen_id++)
		vep_num[screen_id] = de_feat_is_support_vep(screen_id);

	for (screen_id = 0; screen_id < device_num; screen_id++)
		for (ch_id = 0; ch_id < vep_num[screen_id]; ch_id++) {
			de_fce_init(screen_id, ch_id,
				    para->reg_base[DISP_MOD_DE]);
#ifdef BWS_EXIST
			de_bws_init(screen_id, ch_id,
				    para->reg_base[DISP_MOD_DE]);
#endif
#ifdef LTI_EXIST
			de_lti_init(screen_id, ch_id,
				    para->reg_base[DISP_MOD_DE]);
#endif
			de_peak_init(screen_id, ch_id,
				     para->reg_base[DISP_MOD_DE]);
#ifdef ASE_EXIST
			de_ase_init(screen_id, ch_id,
				    para->reg_base[DISP_MOD_DE]);
#endif
#ifdef FCC_EXIST
			de_fcc_init(screen_id, ch_id,
				    para->reg_base[DISP_MOD_DE]);
#endif
		}

	/* initial */
	for (screen_id = 0; screen_id < device_num; screen_id++)
		frame_cnt[screen_id] = 0;

	return 0;
}

int de_enhance_set_mode(struct disp_enhance_config *config)
{
	unsigned int mode;
	/* FIXME: 1.definition here, right? 2.mode 1/ mode 2 should be different parameters */
	unsigned int enhance_mode_to_para[MODE_NUM][PARA_NUM] = {	/* SHARP    //AUTO_CONTRAST  //AUTO_COLOR  //FC_R    //FC_G    //FC_B */
		{3, 2, 1, 1, 1, 1},	/* lcd */
		{3, 3, 1, 1, 2, 1},	/* hdmi */
	};

	mode = config->info.mode;

	if (mode >= MODE_NUM) {
		__wrn
		    ("de_enhance_set_mode : enhance mode excess maximum mode number!! mode = %d, max_mode = %d.\n",
		     mode, MODE_NUM);
		return 1;
	}

	config->info.sharp = enhance_mode_to_para[mode][0];
	config->info.auto_contrast = enhance_mode_to_para[mode][1];
	config->info.auto_color = enhance_mode_to_para[mode][2];
	config->info.fancycolor_red = enhance_mode_to_para[mode][3];
	config->info.fancycolor_green = enhance_mode_to_para[mode][4];
	config->info.fancycolor_blue = enhance_mode_to_para[mode][5];

	return 0;
}
