/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

/******************************************************************************
 *  All Winner Tech, All Right Reserved. 2014-2015 Copyright (c)
 *
 *  File name   :       de_enhance.c
 *
 *  Description :       display engine 2.0 enhance basic function definition
 *
 *  History     :       2014/04/29  vito cheng  v0.1  Initial version
 *
 *****************************************************************************/
#include "de_rtmx.h"
#include "de_enhance.h"
#include "de_feat.h"
#ifdef CONFIG_AW_DISP2_SUPPORT_ENAHNCE
#include "de_vep_table.h"

#define ONE_SCREEN_ONE_PARA
/*
 * only ONE parameters for one screen,
 * and all VEPs in this screen use SAME parameters
 */

/* enhance configuration */
static struct vep_config_info *g_cfg;
/* enhance channel layer information */
static struct disp_enhance_chn_info *g_chn_info[DE_NUM][VI_CHN_NUM];
/* enhance channel module information */
static struct vep_para *g_para[DE_NUM][VI_CHN_NUM];
/* enhance algorithm variable */
static struct vep_alg_var *g_alg[DE_NUM][VI_CHN_NUM];

static int de_enhance_init_para(unsigned int screen_id, unsigned int ch_id)
{
	if (g_cfg->dns_exist[screen_id][ch_id])
		de_dns_init_para(screen_id, ch_id);
	if (g_cfg->peak2d_exist[screen_id][ch_id])
		de_peak2d_init_para(screen_id, ch_id);
	de_fce_init_para(screen_id, ch_id);
	de_peak_init_para(screen_id, ch_id);
	de_lti_init_para(screen_id, ch_id);
	de_fcc_init_para(screen_id, ch_id);
	de_bls_init_para(screen_id, ch_id);

	return 0;
}

#if 0
static int de_enhance_enable(unsigned int screen_id, unsigned int ch_id,
			     unsigned int enable)
{
	de_fce_enable(screen_id, ch_id, enable);
	de_peak_enable(screen_id, ch_id, enable);
	if (g_cfg->peak2d_exist[screen_id][ch_id])
		de_peak2d_enable(screen_id, ch_id, enable);
	de_lti_enable(screen_id, ch_id, enable);
	de_fcc_enable(screen_id, ch_id, enable);
	de_bls_enable(screen_id, ch_id, enable);
	if (g_cfg->dns_exist[screen_id][ch_id])
		de_dns_enable(screen_id, ch_id, enable);

	return 0;

}
#endif

static int de_enhance_set_size(unsigned int screen_id, unsigned int ch_id)
{
	struct de_rect tmp_win; /* bld size window */
	struct de_rect tmp_win2;/* ovl size window */
	struct disp_rectsz size; /* bld size */
	struct disp_rectsz size2; /* ovl size */
	unsigned int demo_enable;

	demo_enable = g_para[screen_id][ch_id]->demo_enable;
	size.width = g_chn_info[screen_id][ch_id]->bld_size.width;
	size.height = g_chn_info[screen_id][ch_id]->bld_size.height;
	size2.width = g_chn_info[screen_id][ch_id]->ovl_size.width;
	size2.height = g_chn_info[screen_id][ch_id]->ovl_size.height;

	tmp_win.x = 0;
	tmp_win.y = 0;
	tmp_win2.x = 0;
	tmp_win2.y = 0;

	if (demo_enable) {
		if (size.width > size.height) {
			tmp_win.w = size.width >> 1;
			tmp_win.h = size.height;
			/* ovl window follow bld window */
			tmp_win2.w = size2.width >> 1;
			tmp_win2.h = size2.height;
		} else {
			tmp_win.w = size.width;
			tmp_win.h = size.height >> 1;
			/* ovl window follow bld window */
			tmp_win2.w = size2.width;
			tmp_win2.h = size2.height >> 1;
		}
	} else {
		tmp_win.w = size.width;
		tmp_win.h = size.height;
		tmp_win2.w = size2.width;
		tmp_win2.h = size2.height;
	}

	/* dns */
	if (g_cfg->dns_exist[screen_id][ch_id]) {
		de_dns_set_size(screen_id, ch_id, size2.width, size2.height);
		de_dns_set_window(screen_id, ch_id, demo_enable, tmp_win2);
	}
	/* fce */
	de_fce_set_size(screen_id, ch_id, size.width, size.height);
	de_fce_set_window(screen_id, ch_id, demo_enable, tmp_win);

	/* peak */
	de_peak_set_size(screen_id, ch_id, size.width, size.height);
	de_peak_set_window(screen_id, ch_id, demo_enable, tmp_win);

	/* lti */
	de_lti_set_size(screen_id, ch_id, size.width, size.height);
	de_lti_set_window(screen_id, ch_id, demo_enable, tmp_win);

	/* bls */
	de_bls_set_size(screen_id, ch_id, size.width, size.height);
	de_bls_set_window(screen_id, ch_id, demo_enable, tmp_win);

	/* fcc */
	de_fcc_set_size(screen_id, ch_id, size.width, size.height);
	de_fcc_set_window(screen_id, ch_id, demo_enable, tmp_win);

	return 0;
}

static int de_enhance_apply_core(unsigned int screen_id)
{
	int ch_id, chno;
	unsigned int other_dirty;
	unsigned int dev_type;
	unsigned int fmt;

	chno = g_cfg->vep_num[screen_id];
	other_dirty = ENH_SIZE_DIRTY | ENH_FORMAT_DIRTY | ENH_ENABLE_DIRTY |
		      ENH_MODE_DIRTY;
	for (ch_id = 0; ch_id < chno; ch_id++) {
		fmt = g_para[screen_id][ch_id]->fmt;
		dev_type = g_para[screen_id][ch_id]->dev_type;

		/* 0. initial parameters */
		if (g_para[screen_id][ch_id]->flags & ENH_INIT_DIRTY)
			de_enhance_init_para(screen_id, ch_id);

		/* 1. size and demo window reg */
		if (g_para[screen_id][ch_id]->flags &
		    (ENH_SIZE_DIRTY | ENH_FORMAT_DIRTY | ENH_MODE_DIRTY))
			de_enhance_set_size(screen_id, ch_id);

		/* 2. module parameters */
		if (g_para[screen_id][ch_id]->flags & (other_dirty |
		    ENH_BRIGHT_DIRTY | ENH_CONTRAST_DIRTY | ENH_BYPASS_DIRTY))
			de_fce_info2para(screen_id, ch_id, fmt, dev_type,
					 &g_para[screen_id][ch_id]->fce_para,
					 g_para[screen_id][ch_id]->bypass);

		if (g_para[screen_id][ch_id]->flags &
		    (other_dirty | ENH_DETAIL_DIRTY | ENH_BYPASS_DIRTY)) {
			if (g_cfg->peak2d_exist[screen_id][ch_id])
				de_peak2d_info2para(screen_id, ch_id, fmt,
					dev_type,
					&g_para[screen_id][ch_id]->peak2d_para,
					g_para[screen_id][ch_id]->bypass);
			de_peak_info2para(screen_id, ch_id, fmt, dev_type,
					&g_para[screen_id][ch_id]->peak_para,
					g_para[screen_id][ch_id]->bypass);
		}

		if (g_para[screen_id][ch_id]->flags &
		    (other_dirty | ENH_EDGE_DIRTY | ENH_BYPASS_DIRTY))
			de_lti_info2para(screen_id, ch_id, fmt, dev_type,
					  &g_para[screen_id][ch_id]->lti_para,
					  g_para[screen_id][ch_id]->bypass);

		if (g_para[screen_id][ch_id]->flags &
		    (ENH_FORMAT_DIRTY | ENH_ENABLE_DIRTY | ENH_MODE_DIRTY |
		     ENH_SAT_DIRTY | ENH_BYPASS_DIRTY))
			de_fcc_info2para(screen_id, ch_id, fmt, dev_type,
					  &g_para[screen_id][ch_id]->fcc_para,
					  g_para[screen_id][ch_id]->bypass);

		if (g_para[screen_id][ch_id]->flags &
		    (ENH_FORMAT_DIRTY | ENH_ENABLE_DIRTY | ENH_MODE_DIRTY |
		     ENH_SAT_DIRTY | ENH_BYPASS_DIRTY))
			de_bls_info2para(screen_id, ch_id, fmt, dev_type,
					 &g_para[screen_id][ch_id]->bls_para,
					 g_para[screen_id][ch_id]->bypass);

		if (g_para[screen_id][ch_id]->flags &
		    (other_dirty | ENH_DNS_DIRTY | ENH_BYPASS_DIRTY) &&
		    g_cfg->dns_exist[screen_id][ch_id])
			de_dns_info2para(screen_id, ch_id, fmt, dev_type,
					  &g_para[screen_id][ch_id]->dns_para,
					  g_para[screen_id][ch_id]->bypass);

		/* clear dirty in g_config */
		g_para[screen_id][ch_id]->flags = 0x0;
	}

	return 0;

}

int de_enhance_layer_apply(unsigned int screen_id,
			   struct disp_enhance_chn_info *info)
{
	int ch_id, chno, i, layno;
	int feid; /* first_enable_layer_id */
	unsigned int overlay_enable = 0;
	unsigned int format = 0xff;
	struct disp_enhance_chn_info *p_info;
	unsigned int bypass;
	enum disp_enhance_dirty_flags flags = {0};

	chno = g_cfg->vep_num[screen_id];

	__inf("%s, ovl_size=<%d,%d>, bld_size=<%d,%d>\n", __func__,
	    info->ovl_size.width, info->ovl_size.height, info->bld_size.width,
	    info->bld_size.height);

	p_info = kmalloc(sizeof(struct disp_enhance_chn_info), GFP_KERNEL |
		 __GFP_ZERO);
	if (p_info == NULL) {
		__wrn("malloc p_info memory fail! size=0x%x\n",
		      (unsigned int)sizeof(struct disp_enhance_chn_info));
		return -1;
	}

	for (ch_id = 0; ch_id < chno; ch_id++) {
		memcpy(p_info, g_chn_info[screen_id][ch_id],
		       sizeof(struct disp_enhance_chn_info));
		/* ENABLE DIRTY */
		layno = de_feat_get_num_layers_by_chn(screen_id, ch_id);
		feid = -1;
		for (i = 0; i < layno; i++) {
			if (info[ch_id].layer_info[i].en !=
			    p_info->layer_info[i].en)
				flags |= ENH_ENABLE_DIRTY;

			if (info[ch_id].layer_info[i].en) {
				overlay_enable |= 0x1;
				if (feid == -1)
					feid = i;
			}
		}

		/* SIZE DIRTY */
		if (((info[ch_id].ovl_size.width != p_info->ovl_size.width) ||
		    (info[ch_id].ovl_size.height != p_info->ovl_size.height) ||
		    (info[ch_id].bld_size.width != p_info->bld_size.width) ||
		    (info[ch_id].bld_size.height != p_info->bld_size.height))
		    && (overlay_enable == 0x1)) {
			flags |= ENH_SIZE_DIRTY;
		} else {
			for (i = 0; i < layno; i++) {
				if (info[ch_id].layer_info[i].en &&
				   ((p_info->layer_info[i].fb_size.width !=
				   info[ch_id].layer_info[i].fb_size.width) ||
				   (p_info->layer_info[i].fb_size.height !=
				   info[ch_id].layer_info[i].fb_size.height) ||
				   (p_info->layer_info[i].fb_crop.x !=
				   info[ch_id].layer_info[i].fb_crop.x) ||
				   (p_info->layer_info[i].fb_crop.y !=
				   info[ch_id].layer_info[i].fb_crop.y)))
					flags |= ENH_SIZE_DIRTY;
			}
		}

		/* FORMAT DIRTY */
		for (i = 0; i < layno; i++) {
			if (info[ch_id].layer_info[i].en &&
			    (p_info->layer_info[i].format !=
			     info[ch_id].layer_info[i].format)) {
				flags |= ENH_FORMAT_DIRTY;
				format = (info[ch_id].layer_info[i].format >=
					  DE_FORMAT_YUV444_I_AYUV) ? 0 : 1;
				break;
			}
		}

		/* UPDATE g_chn_info */
		if (flags &
		    (ENH_ENABLE_DIRTY | ENH_SIZE_DIRTY | ENH_FORMAT_DIRTY))
			memcpy(g_chn_info[screen_id][ch_id], &info[ch_id],
				sizeof(struct disp_enhance_chn_info));

		/* UPDATE g_para->foramt */
		if (flags & ENH_FORMAT_DIRTY)
			g_para[screen_id][ch_id]->fmt = format;

		/* UPDATE g_para->bypass */
		/* Old bypass */
		bypass = g_para[screen_id][ch_id]->bypass;
		if (flags & ENH_SIZE_DIRTY) {
			if (g_chn_info[screen_id][ch_id]->ovl_size.width <
			    ENHANCE_MIN_OVL_WIDTH ||
			    g_chn_info[screen_id][ch_id]->ovl_size.height <
			    ENHANCE_MIN_OVL_HEIGHT ||
			    g_chn_info[screen_id][ch_id]->bld_size.width <
			    ENHANCE_MIN_BLD_WIDTH ||
			    g_chn_info[screen_id][ch_id]->bld_size.height <
			    ENHANCE_MIN_BLD_HEIGHT)
				g_para[screen_id][ch_id]->bypass |=
					SIZE_BYPASS;
			else
				g_para[screen_id][ch_id]->bypass &=
					SIZE_BYPASS_MASK;
		}

		if (flags & ENH_ENABLE_DIRTY) {
			if (!overlay_enable)
				g_para[screen_id][ch_id]->bypass |=
					LAYER_BYPASS;
			else
				g_para[screen_id][ch_id]->bypass &=
					LAYER_BYPASS_MASK;
		}

		/* BYPASS_DIRTY */
		if (bypass != g_para[screen_id][ch_id]->bypass)
			flags |= ENH_BYPASS_DIRTY;

		g_para[screen_id][ch_id]->flags = flags;

		/* UPDATE g_para->module_para */
		if (flags & ENH_SIZE_DIRTY) {
			/* peak_para */
			g_para[screen_id][ch_id]->peak_para.inw =
				g_chn_info[screen_id][ch_id]->ovl_size.width;
			g_para[screen_id][ch_id]->peak_para.inh =
				g_chn_info[screen_id][ch_id]->ovl_size.height;
			g_para[screen_id][ch_id]->peak_para.outw =
				g_chn_info[screen_id][ch_id]->bld_size.width;
			g_para[screen_id][ch_id]->peak_para.outh =
				g_chn_info[screen_id][ch_id]->bld_size.height;
			/* peak2d_para */
			g_para[screen_id][ch_id]->peak2d_para.inw =
				g_chn_info[screen_id][ch_id]->ovl_size.width;
			g_para[screen_id][ch_id]->peak2d_para.inh =
				g_chn_info[screen_id][ch_id]->ovl_size.height;
			g_para[screen_id][ch_id]->peak2d_para.outw =
				g_chn_info[screen_id][ch_id]->bld_size.width;
			g_para[screen_id][ch_id]->peak2d_para.outh =
				g_chn_info[screen_id][ch_id]->bld_size.height;
			/* lti_para */
			g_para[screen_id][ch_id]->lti_para.inw =
				g_chn_info[screen_id][ch_id]->ovl_size.width;
			g_para[screen_id][ch_id]->lti_para.inh =
				g_chn_info[screen_id][ch_id]->ovl_size.height;
			g_para[screen_id][ch_id]->lti_para.outw =
				g_chn_info[screen_id][ch_id]->bld_size.width;
			g_para[screen_id][ch_id]->lti_para.outh =
				g_chn_info[screen_id][ch_id]->bld_size.height;
			/* fce_para */
			g_para[screen_id][ch_id]->fce_para.outw =
				g_chn_info[screen_id][ch_id]->bld_size.width;
			g_para[screen_id][ch_id]->fce_para.outh =
				g_chn_info[screen_id][ch_id]->bld_size.height;
			/* dns_para */
			if (feid >= 0) {
				g_para[screen_id][ch_id]->dns_para.inw =
					g_chn_info[screen_id][ch_id]->
						layer_info[feid].fb_size.width;
				g_para[screen_id][ch_id]->dns_para.inh =
					g_chn_info[screen_id][ch_id]->
						layer_info[feid].fb_size.height;
				memcpy(&g_para[screen_id][ch_id]->
				       dns_para.croprect,
				       &g_chn_info[screen_id][ch_id]->
				       layer_info[feid].fb_crop,
				       sizeof(struct disp_rect));
			} else {
				/* all layer disable */
				g_para[screen_id][ch_id]->dns_para.inw = 0;
				g_para[screen_id][ch_id]->dns_para.inh = 0;
				memset(&g_para[screen_id][ch_id]->
				       dns_para.croprect,
				       0, sizeof(struct disp_rect));
			}
		}
	}

	de_enhance_apply_core(screen_id);
	kfree(p_info);
	return 0;
}

int de_enhance_apply(unsigned int screen_id,
			   struct disp_enhance_config *config)
{
	int ch_id, chno;
	unsigned int bypass;

	chno = g_cfg->vep_num[screen_id];

	for (ch_id = 0; ch_id < chno; ch_id++) {
		/* copy dirty to g_para->flags */
		g_para[screen_id][ch_id]->flags |=
					(config[0].flags & ENH_USER_DIRTY);
		bypass = g_para[screen_id][ch_id]->bypass;
		if (config[0].flags & ENH_MODE_DIRTY) {
			/* enhance_mode */
			if (((config[0].info.mode & 0xffff0000) >> 16) == 2) {
				g_para[screen_id][ch_id]->demo_enable = 0x1;
				g_para[screen_id][ch_id]->bypass &=
							USER_BYPASS_MASK;
			} else if (((config[0].info.mode & 0xffff0000) >> 16)
				   == 1) {
				g_para[screen_id][ch_id]->demo_enable = 0x0;
				g_para[screen_id][ch_id]->bypass &=
							USER_BYPASS_MASK;
			} else {
				g_para[screen_id][ch_id]->demo_enable = 0x0;
				g_para[screen_id][ch_id]->bypass |= USER_BYPASS;
			}
			/* dev_type */
			g_para[screen_id][ch_id]->dev_type =
					config[0].info.mode & 0x0000ffff;

			if (bypass != g_para[screen_id][ch_id]->bypass)
				g_para[screen_id][ch_id]->flags |=
							       ENH_BYPASS_DIRTY;
		}

		if (config[0].flags & ENH_BRIGHT_DIRTY) {
			g_para[screen_id][ch_id]->fce_para.bright_level =
						config[0].info.bright;
		}

		if (config[0].flags & ENH_CONTRAST_DIRTY) {
			g_para[screen_id][ch_id]->fce_para.contrast_level =
						config[0].info.contrast;
		}

		if (config[0].flags & ENH_EDGE_DIRTY) {
			g_para[screen_id][ch_id]->lti_para.level =
						config[0].info.edge;
		}

		if (config[0].flags & ENH_DETAIL_DIRTY) {
			g_para[screen_id][ch_id]->peak_para.level =
						config[0].info.detail;
			g_para[screen_id][ch_id]->peak2d_para.level =
						config[0].info.detail;
		}

		if (config[0].flags & ENH_SAT_DIRTY) {
			g_para[screen_id][ch_id]->fcc_para.level =
						config[0].info.saturation;
			g_para[screen_id][ch_id]->bls_para.level =
						config[0].info.saturation;
		}

		if (config[0].flags & ENH_DNS_DIRTY) {
			g_para[screen_id][ch_id]->dns_para.level =
						config[0].info.denoise;
		}
	}

	de_enhance_apply_core(screen_id);
	return 0;
}

int de_enhance_sync(unsigned int screen_id)
{
	/* de_enhance_update_regs(id); */
	return 0;
}

int de_enhance_tasklet(unsigned int screen_id)
{
	int ch_id, chno;

	chno = g_cfg->vep_num[screen_id];

	for (ch_id = 0; ch_id < chno; ch_id++) {
		/* hist */
		de_hist_tasklet(screen_id, ch_id,
				g_alg[screen_id][ch_id]->frame_cnt);

		/* ce */
		de_ce_tasklet(screen_id, ch_id,
			      g_alg[screen_id][ch_id]->frame_cnt);

		/* dns */
		if (g_cfg->dns_exist[screen_id][ch_id])
			de_dns_tasklet(screen_id, ch_id,
				       g_alg[screen_id][ch_id]->frame_cnt);

		g_alg[screen_id][ch_id]->frame_cnt++;
	}
	return 0;
}

int de_enhance_update_regs(unsigned int screen_id)
{
	int chno, ch_id;

	chno = g_cfg->vep_num[screen_id];
	for (ch_id = 0; ch_id < chno; ch_id++) {
		if (g_cfg->dns_exist[screen_id][ch_id])
			de_dns_update_regs(screen_id, ch_id);
		if (g_cfg->peak2d_exist[screen_id][ch_id])
			de_peak2d_update_regs(screen_id, ch_id);

		de_fce_update_regs(screen_id, ch_id);
		de_peak_update_regs(screen_id, ch_id);
		de_lti_update_regs(screen_id, ch_id);
		de_bls_update_regs(screen_id, ch_id);
		de_fcc_update_regs(screen_id, ch_id);
	}
	return 0;
}

int de_enhance_init(struct disp_bsp_init_para *para)
{
	int screen_id, ch_id;
	int i;

	g_cfg = kmalloc(sizeof(struct vep_config_info), GFP_KERNEL |
		 __GFP_ZERO);
	if (g_cfg == NULL) {
		__wrn("malloc g_cfg memory fail! size=0x%x\n",
		      (unsigned int)sizeof(struct vep_config_info));
		return -1;
	}

	g_cfg->device_num = de_feat_get_num_screens();

	for (screen_id = 0; screen_id < g_cfg->device_num; screen_id++)
		g_cfg->vep_num[screen_id] = de_feat_is_support_vep(screen_id);

	for (screen_id = 0; screen_id < g_cfg->device_num; screen_id++)
		for (ch_id = 0; ch_id < g_cfg->vep_num[screen_id]; ch_id++) {
			g_cfg->dns_exist[screen_id][ch_id] =
			   de_feat_is_support_de_noise_by_chn(screen_id, ch_id);
			g_cfg->peak2d_exist[screen_id][ch_id] =
			    de_feat_is_support_edscale_by_chn(screen_id, ch_id);
			if (g_cfg->dns_exist[screen_id][ch_id])
				de_dns_init(screen_id, ch_id,
					    para->reg_base[DISP_MOD_DE]);
			if (g_cfg->peak2d_exist[screen_id][ch_id])
				de_peak2d_init(screen_id, ch_id,
					       para->reg_base[DISP_MOD_DE]);
			de_fce_init(screen_id, ch_id,
				    para->reg_base[DISP_MOD_DE]);
			de_peak_init(screen_id, ch_id,
				     para->reg_base[DISP_MOD_DE]);
			de_lti_init(screen_id, ch_id,
				    para->reg_base[DISP_MOD_DE]);
			de_bls_init(screen_id, ch_id,
				    para->reg_base[DISP_MOD_DE]);
			de_fcc_init(screen_id, ch_id,
				    para->reg_base[DISP_MOD_DE]);

			/* initial local variable */
			g_chn_info[screen_id][ch_id] =
				kmalloc(sizeof(struct disp_enhance_chn_info),
					GFP_KERNEL | __GFP_ZERO);
			if (g_chn_info[screen_id][ch_id] == NULL) {
				__wrn("malloc g_chn_info[%d][%d] memory fail!",
				      screen_id, ch_id);
				__wrn("size=0x%x\n", (unsigned int)
				      sizeof(struct disp_enhance_chn_info));
				return -1;
			}
			for (i = 0; i < LAYER_MAX_NUM_PER_CHN; i++)
				g_chn_info[screen_id][ch_id]->layer_info[i].format = 0xff;

			g_para[screen_id][ch_id] =
				kmalloc(sizeof(struct vep_para), GFP_KERNEL |
					__GFP_ZERO);
			if (g_para[screen_id][ch_id] == NULL) {
				__wrn("malloc g_para[%d][%d] memory fail!",
				      screen_id, ch_id);
				__wrn("size=0x%x\n",
				      (unsigned int)sizeof(struct vep_para));
				return -1;
			}

			g_alg[screen_id][ch_id] =
				kmalloc(sizeof(struct vep_alg_var),
					GFP_KERNEL | __GFP_ZERO);
			if (g_alg[screen_id][ch_id] == NULL) {
				__wrn("malloc g_alg[%d][%d] memory fail!",
				      screen_id, ch_id);
				__wrn("size=0x%x\n",
				      (unsigned int)sizeof(struct vep_alg_var));
				return -1;
			}

			g_para[screen_id][ch_id]->peak_para.peak2d_exist =
				g_cfg->peak2d_exist[screen_id][ch_id];
		}

	return 0;
}

int de_enhance_exit(void)
{
	int screen_id, ch_id;

	for (screen_id = 0; screen_id < g_cfg->device_num; screen_id++)
		for (ch_id = 0; ch_id < g_cfg->vep_num[screen_id]; ch_id++) {
			if (g_cfg->dns_exist[screen_id][ch_id])
				de_dns_exit(screen_id, ch_id);
			if (g_cfg->peak2d_exist[screen_id][ch_id])
				de_peak2d_exit(screen_id, ch_id);
			de_fce_exit(screen_id, ch_id);
			de_peak_exit(screen_id, ch_id);
			de_lti_exit(screen_id, ch_id);
			de_bls_exit(screen_id, ch_id);
			de_fcc_exit(screen_id, ch_id);

			kfree(g_chn_info[screen_id][ch_id]);
			kfree(g_para[screen_id][ch_id]);
			kfree(g_alg[screen_id][ch_id]);
		}

	kfree(g_cfg);

	return 0;
}

int de_enhance_double_init(struct disp_bsp_init_para *para)
{
	int screen_id, ch_id;

	g_cfg = kmalloc(sizeof(struct vep_config_info), GFP_KERNEL |
		 __GFP_ZERO);
	if (g_cfg == NULL) {
		__wrn("malloc g_cfg memory fail! size=0x%x\n",
		      (unsigned int)sizeof(struct vep_config_info));
		return -1;
	}

	g_cfg->device_num = de_feat_get_num_screens();

	for (screen_id = 0; screen_id < g_cfg->device_num; screen_id++)
		g_cfg->vep_num[screen_id] = de_feat_is_support_vep(screen_id);

	for (screen_id = 0; screen_id < g_cfg->device_num; screen_id++)
		for (ch_id = 0; ch_id < g_cfg->vep_num[screen_id]; ch_id++) {
			g_cfg->dns_exist[screen_id][ch_id] =
			   de_feat_is_support_de_noise_by_chn(screen_id, ch_id);
			g_cfg->peak2d_exist[screen_id][ch_id] =
			    de_feat_is_support_edscale_by_chn(screen_id, ch_id);
			if (g_cfg->dns_exist[screen_id][ch_id])
				de_dns_init(screen_id, ch_id,
					    para->reg_base[DISP_MOD_DE]);
			if (g_cfg->peak2d_exist[screen_id][ch_id])
				de_peak2d_init(screen_id, ch_id,
					       para->reg_base[DISP_MOD_DE]);
			de_fce_double_init(screen_id, ch_id,
					   para->reg_base[DISP_MOD_DE]);
			de_peak_double_init(screen_id, ch_id,
					    para->reg_base[DISP_MOD_DE]);
			de_lti_double_init(screen_id, ch_id,
					   para->reg_base[DISP_MOD_DE]);
			de_bls_double_init(screen_id, ch_id,
					   para->reg_base[DISP_MOD_DE]);
			de_fcc_double_init(screen_id, ch_id,
					   para->reg_base[DISP_MOD_DE]);

			/* initial local variable */
			g_chn_info[screen_id][ch_id] =
				kmalloc(sizeof(struct disp_enhance_chn_info),
					GFP_KERNEL | __GFP_ZERO);
			if (g_chn_info[screen_id][ch_id] == NULL) {
				__wrn("malloc g_chn_info[%d][%d] memory fail!",
				      screen_id, ch_id);
				__wrn("size=0x%x\n", (unsigned int)
				      sizeof(struct disp_enhance_chn_info));
				return -1;
			}

			g_para[screen_id][ch_id] =
				kmalloc(sizeof(struct vep_para), GFP_KERNEL |
					__GFP_ZERO);
			if (g_para[screen_id][ch_id] == NULL) {
				__wrn("malloc g_para[%d][%d] memory fail!",
				      screen_id, ch_id);
				__wrn("size=0x%x\n",
				      (unsigned int)sizeof(struct vep_para));
				return -1;
			}

			g_alg[screen_id][ch_id] =
				kmalloc(sizeof(struct vep_alg_var),
					GFP_KERNEL | __GFP_ZERO);
			if (g_alg[screen_id][ch_id] == NULL) {
				__wrn("malloc g_alg[%d][%d] memory fail!",
				      screen_id, ch_id);
				__wrn("size=0x%x\n",
				      (unsigned int)sizeof(struct vep_alg_var));
				return -1;
			}

			g_para[screen_id][ch_id]->peak_para.peak2d_exist =
				g_cfg->peak2d_exist[screen_id][ch_id];
		}

	return 0;
}

int de_enhance_double_exit(void)
{
	int screen_id, ch_id;

	for (screen_id = 0; screen_id < g_cfg->device_num; screen_id++)
		for (ch_id = 0; ch_id < g_cfg->vep_num[screen_id]; ch_id++) {
			if (g_cfg->dns_exist[screen_id][ch_id])
				de_dns_double_exit(screen_id, ch_id);
			if (g_cfg->peak2d_exist[screen_id][ch_id])
				de_peak2d_double_exit(screen_id, ch_id);
			de_fce_double_exit(screen_id, ch_id);
			de_peak_double_exit(screen_id, ch_id);
			de_lti_double_exit(screen_id, ch_id);
			de_bls_double_exit(screen_id, ch_id);
			de_fcc_double_exit(screen_id, ch_id);

			kfree(g_chn_info[screen_id][ch_id]);
			kfree(g_para[screen_id][ch_id]);
			kfree(g_alg[screen_id][ch_id]);
		}

	kfree(g_cfg);

	return 0;
}

void de_set_bits(unsigned int *reg_addr, unsigned int bits_val,
			unsigned int shift, unsigned int width)
{
	unsigned int reg_val;

	reg_val = de_readl(reg_addr);
	reg_val = SET_BITS(shift, width, reg_val, bits_val);
	de_writel(reg_val, reg_addr);
}
#else
int de_enhance_layer_apply(unsigned int screen_id,
			   struct disp_enhance_chn_info *info)
{
	return 0;
}

int de_enhance_apply(unsigned int screen_id,
			   struct disp_enhance_config *config)
{
	return 0;
}

int de_enhance_sync(unsigned int screen_id)
{
	/* de_enhance_update_regs(id); */
	return 0;
}
int de_enhance_tasklet(unsigned int screen_id)
{
	return 0;
}

int de_enhance_update_regs(unsigned int screen_id)
{
	return 0;
}

int de_enhance_init(struct disp_bsp_init_para *para)
{
	return 0;
}
int de_enhance_exit(void)
{
	return 0;
}

int de_enhance_double_init(struct disp_bsp_init_para *para)
{
	return 0;
}

int de_enhance_double_exit(void)
{
	return 0;
}

void de_set_bits(unsigned int *reg_addr, unsigned int bits_val,
			unsigned int shift, unsigned int width)
{
}
#endif
