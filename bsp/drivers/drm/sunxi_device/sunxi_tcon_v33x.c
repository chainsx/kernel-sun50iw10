/*sunxi_tcon_v33x.c
 *
 * Copyright (C) 2022 Allwinnertech Co., Ltd.
 * Authors: zhengwanyu <zhengwanyu@allwinnertech.com>
 * Authors: hongyaobin <hongyaobin@allwinnertech.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/dma-mapping.h>
#include <linux/clk.h>
#include <drm/drm_print.h>

#include "sunxi_tcon.h"

#include "tcon_feat.h"
#include "disp_al_tcon.h"

#include "disp_al.h"

#include "sunxi_common.h"
#include "sunxi_lcd.h"


static struct sunxi_tcon_drv *tcon_drv;

struct sunxi_dispdev_name sunxi_output_type_name[] = {
	{DISP_OUTPUT_TYPE_LCD, "LCD"},
	{DISP_OUTPUT_TYPE_TV, "TVE"},
	{DISP_OUTPUT_TYPE_HDMI, "HDMI"},
	{DISP_OUTPUT_TYPE_VGA, "VGA"},
	{DISP_OUTPUT_TYPE_VDPO, "VDPO"},
	{DISP_OUTPUT_TYPE_EDP, "EDP"},
};

static char *sunxi_tcon_get_output_type_name(int output_type)
{
	int i, cnt;

	cnt  = sizeof(sunxi_output_type_name)
			/ sizeof(struct sunxi_dispdev_name);

	for (i = 0; i < cnt; i++)
		if (sunxi_output_type_name[i].device == output_type)
			return sunxi_output_type_name[i].name;

	return NULL;
}

struct sunxi_tcon *sunxi_tcon_get_tcon(int nr)
{
	return &tcon_drv->hwtcon[nr];
}

void sunxi_tcon_pad_sel(unsigned int enc_id, unsigned int en)
{
	struct sunxi_tcon *hwtcon = sunxi_tcon_get_tcon(enc_id);

	if (!__clk_get_enable_count(hwtcon->mclk))
		clk_prepare_enable(hwtcon->mclk);
	/* disp_al_hdmi_pad_sel(enc_id, en); */
	tcon_pan_sel(enc_id, en);
}

static bool sunxi_tcon_is_use_irq(unsigned int enc_id)
{
	struct sunxi_tcon *hwtcon = sunxi_tcon_get_tcon(enc_id);

	return hwtcon->irq_no ? true : false;
}

static unsigned int sunxi_tcon_get_irq_no(unsigned int enc_id)
{
	struct sunxi_tcon *hwtcon = sunxi_tcon_get_tcon(enc_id);

	return hwtcon->irq_no;
}

#ifdef CONFIG_AW_DRM_LCD
int sunxi_tcon_lcd_enable(int nr, int lcd_id)
{
	enum disp_lcd_if lcd_if;
	struct disp_panel_para *panel
		= sunxi_lcd_get_panel_para(lcd_id);
	int type_id = 0;

	type_id = sunxi_lcd_get_type_id(lcd_id);

	lcd_if = sunxi_lcd_get_lcd_if(lcd_id);

	tcon0_open(nr, panel);
	if (panel->lcd_if == LCD_IF_LVDS) {
		lvds_open(type_id, panel);
	} else if (panel->lcd_if == LCD_IF_DSI) {
#if defined(SUPPORT_DSI)
		dsi_open(type_id, panel);
		if (panel->lcd_tcon_mode == DISP_TCON_DUAL_DSI
			&& nr + 1 < DEVICE_DSI_NUM)
			dsi_open(type_id + 1, panel);
#endif
	}

	return 0;
}

int sunxi_tcon_lcd_disable(int nr, int lcd_id)
{
	enum disp_lcd_if lcd_if;
	struct disp_panel_para *panel
		= sunxi_lcd_get_panel_para(lcd_id);
	int type_id = 0;

	type_id = sunxi_lcd_get_type_id(lcd_id);

	lcd_if = sunxi_lcd_get_lcd_if(lcd_id);

	if (panel->lcd_if == LCD_IF_LVDS) {
		lvds_close(nr);
	} else if (panel->lcd_if == LCD_IF_DSI) {
#if defined(SUPPORT_DSI)
		dsi_close(type_id);
		if (panel->lcd_tcon_mode == DISP_TCON_DUAL_DSI &&
		    nr + 1 < DEVICE_DSI_NUM)
			dsi_close(type_id + 1);
#endif
	}
	tcon0_close(nr);

	return 0;
}

static int sunxi_tcon_lcd_query_irq(int nr)
{
	int lcd_id, type_id = 0;
	enum disp_lcd_if lcd_if;
	struct sunxi_tcon *hwtcon = sunxi_tcon_get_tcon(nr);

	if (hwtcon->type != TCON_LCD) {
		DRM_ERROR("wrong TCON type\n");
		return -1;
	}

	lcd_id = hwtcon->conn_type_id;

	type_id = sunxi_lcd_get_type_id(lcd_id);

	lcd_if = sunxi_lcd_get_lcd_if(lcd_id);
	if (sunxi_lcd_is_use_irq(lcd_id)) {
#if defined(SUPPORT_DSI)
		if (lcd_if == LCD_IF_DSI)
			return dsi_irq_query(type_id,
					DSI_IRQ_VIDEO_VBLK);
#endif
	} else if (sunxi_tcon_is_use_irq(nr)) {
		/* DRM_INFO("tcon%d irq query\n", nr); */
		return tcon_irq_query(nr,
				LCD_IRQ_TCON0_VBLK);
	}

	return 0;
}
#endif

/**
 * @name       :sunxi_tcon_device_query_irq
 * @brief      :query irq status(tconlcd or tcontv)
 * @param[IN]  :nr:index of tcon
 * @return     :irq status
 */
static int sunxi_tcon_device_query_irq(int nr)
{
	int ret = 0;
	int irq_id = 0;
	struct sunxi_tcon *hwtcon = sunxi_tcon_get_tcon(nr);

	irq_id = (hwtcon->type == TCON_LCD) ? LCD_IRQ_TCON0_VBLK
					    : LCD_IRQ_TCON1_VBLK;
	ret = tcon_irq_query(nr, irq_id);

	return ret;
}

int sunxi_tcon_query_irq(int nr)
{
	struct sunxi_tcon *hwtcon = sunxi_tcon_get_tcon(nr);

	if (hwtcon->type == TCON_LCD)
#ifdef CONFIG_AW_DRM_LCD
		return sunxi_tcon_lcd_query_irq(nr);
#else
		;
#endif
	else if (hwtcon->type == TCON_TV)
		return sunxi_tcon_device_query_irq(nr);

	DRM_ERROR("WRONG TCON Type\n");
	return -1;
}

/* return the tcon count */
int sunxi_tcon_get_count(void)
{
	return tcon_drv->tcon_cnt;
}

/* Check if this tcon support that con_type */
static bool sunxi_tcon_type_is_support(int nr, int con_type)
{
	struct sunxi_tcon *hwtcon;

	hwtcon = &tcon_drv->hwtcon[nr];

	if ((con_type == DISP_OUTPUT_TYPE_LCD)
		|| (con_type == DISP_OUTPUT_TYPE_EDP)) {
		if (hwtcon->type == TCON_LCD)
			return true;
		else
			return false;
	} else if ((con_type == DISP_OUTPUT_TYPE_HDMI)
		|| (con_type == DISP_OUTPUT_TYPE_TV)) {
		if (hwtcon->type == TCON_TV)
			return true;
		else
			return false;
	}

	return false;
}

static ssize_t tcon_info_show(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	ssize_t i, n = 0;
	struct sunxi_tcon *hwtcon;

#if defined(HAVE_DEVICE_COMMON_MODULE)
	n += sprintf(buf + n, "%s\n", "disp_if_top info:");
	n += sprintf(buf + n, "virt_reg_base: 0x%lx\n",
				tcon_drv->hwtopif.reg_base);
	n += sprintf(buf + n, "clk_name:%s clk_rate: %lu  enable count:%u\n\n",
				__clk_get_name(tcon_drv->hwtopif.mclk),
				clk_get_rate(tcon_drv->hwtopif.mclk),
				__clk_get_enable_count(tcon_drv->hwtopif.mclk));
#endif

	n += sprintf(buf + n, "%s\n", "sunxi-tcon info:");
	n += sprintf(buf + n, "tcon count:%u tcon_lcd_count:%u tcon_tv_count:%u\n\n",
		tcon_drv->tcon_cnt, tcon_drv->tcon_lcd_cnt, tcon_drv->tcon_tv_cnt);

	for (i = 0; i < tcon_drv->tcon_cnt; i++) {
		hwtcon = &tcon_drv->hwtcon[i];
		n += sprintf(buf + n, "id: %d\n", hwtcon->id);

		n += sprintf(buf + n, "virt_reg_base: 0x%lx\n",
						hwtcon->reg_base);

		if (hwtcon->type == TCON_LCD)
			n += sprintf(buf + n, "TCON_LCD%d\n", hwtcon->type_id);
		else if (hwtcon->type == TCON_TV)
			n += sprintf(buf + n, "TCON_TV%d\n", hwtcon->type_id);
		else
			n += sprintf(buf + n, "ERROR: Unknowed TCON_TYPE:%d",
								hwtcon->type);

		n += sprintf(buf + n, "clk_name:%s clk_rate: %lu  enable count:%d\n",
					__clk_get_name(hwtcon->mclk),
					clk_get_rate(hwtcon->mclk),
					__clk_get_enable_count(hwtcon->mclk));

		n += sprintf(buf + n, "irq no: %u\n", hwtcon->irq_no);

		if (hwtcon->is_assigned)
			n += sprintf(buf + n, "%s\n", "Has been assigned");
		else
			n += sprintf(buf + n, "%s\n", "Has NOT been assigned");
		n += sprintf(buf + n, "%s", "\n");
	}

	return n;
}

static ssize_t tcon_info_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	return count;
}

static DEVICE_ATTR(info, 0660, tcon_info_show, tcon_info_store);

static ssize_t tcon_reg_show(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	ssize_t n = 0;
	unsigned int *reg, i, j;
	struct sunxi_tcon *hwtcon;

	for (i = 0; i < tcon_drv->tcon_cnt; i++) {
		hwtcon = &tcon_drv->hwtcon[i];
		if (!__clk_is_enabled(hwtcon->mclk)) {
			n += sprintf(buf + n, "tcon(id = %d) clk is NOT enabled,"
				"can NOT dump reg\n", hwtcon->id);
			if (i >= tcon_drv->tcon_cnt)
				return n;
			continue;
		}

		n += sprintf(buf + n, "tcon(id = %d):\n", hwtcon->id);
		reg = (unsigned int *)hwtcon->reg_base;
		for (j = 0; j < 0x80; j++) {
			/* print address info */
			if (j == 0)
				n += sprintf(buf + n, "0x%04x:", j * 4);
			if ((j % 4 == 0) && (j != 0)) {
				n += sprintf(buf + n, "%s", "\n");
				n += sprintf(buf + n, "0x%04x:", j * 4);
			}

			/* print reg value */
			n += sprintf(buf + n, "0x%08x ", reg[j]);
		}
		n += sprintf(buf + n, "%s", "\n");
	}

	return n;
}

static ssize_t tcon_reg_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	return count;
}

static DEVICE_ATTR(tcon_reg, 0660, tcon_reg_show, tcon_reg_store);

#if defined(HAVE_DEVICE_COMMON_MODULE)
static ssize_t topif_reg_show(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	ssize_t n = 0;
	unsigned int *reg, j;
	struct sunxi_top_if *topif;

	topif = &tcon_drv->hwtopif;
	if (!__clk_is_enabled(topif->mclk)) {
		n += sprintf(buf + n, "disp_top_if clk is NOT enabled,"
				"can NOT dump reg\n");
		return n;
	}

	n += sprintf(buf + n, "disp_top_if:\n");
	reg = (unsigned int *)topif->reg_base;
	for (j = 0; j < 0x40; j++) {
		/* print address info */
		if (j == 0)
			n += sprintf(buf + n, "0x%04x:", j * 4);
		if ((j % 4 == 0) && (j != 0)) {
			n += sprintf(buf + n, "%s", "\n");
			n += sprintf(buf + n, "0x%04x:", j * 4);
		}

		/* print reg value */
		n += sprintf(buf + n, "0x%08x ", reg[j]);
	}
	n += sprintf(buf + n, "%s", "\n");

	return n;
}

static ssize_t topif_reg_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	return count;
}

static DEVICE_ATTR(topif_reg, 0660, topif_reg_show, topif_reg_store);
#endif

static struct attribute *tcon_attributes[] = {
	&dev_attr_info.attr,
	&dev_attr_tcon_reg.attr,
#if defined(HAVE_DEVICE_COMMON_MODULE)
	&dev_attr_topif_reg.attr,
#endif
	NULL
};

static struct attribute_group tcon_attribute_group = {
	.name = "attr",
	.attrs = tcon_attributes,
};

/* judge if this encoder support this connector
 * @encoder: id of the encoder
 * @output_type: connector output type
 */
int sunxi_drm_encoder_support(int encoder, enum disp_output_type output_type)
{
	return de_feat_is_supported_output_types(encoder, output_type);
}

/*
 * NOTE:if you want to use abnormal dsi working mode,
 * you have to implement this function
 */
unsigned int sunxi_tcon_mode(unsigned int id)
{
	return 0;
}

/*
 * NOTE:if you want to use abnormal dsi working mode,
 * you have to implement this function
 */
unsigned int sunxi_tcon_slave_num(unsigned int id)
{
	return 0;
}

/*
 * NOTE:if you want to use abnormal dsi working mode,
 * you have to implement this function
 */
unsigned int sunxi_tcon_port_num(unsigned int id)
{
	return 0;
}

#ifdef CONFIG_AW_DRM_LCD
static bool sunxi_tcon_lcd_is_support(int nr, int lcd_id)
{
	int lcd_if;

	if (!sunxi_tcon_type_is_support(nr, DISP_OUTPUT_TYPE_LCD))
		return false;

	lcd_if = sunxi_lcd_get_lcd_if(lcd_id);

	return 1; /* tcon_lcd always support */
}

int sunxi_tcon_lcd_get_tcon_id(unsigned int lcd_id)
{
	unsigned int tcon_cnt, i;
	struct sunxi_tcon *hwtcon;

	tcon_cnt = sunxi_tcon_get_count();
	for (i = 0; i < tcon_cnt; i++) {
		hwtcon = sunxi_tcon_get_tcon(i);
		if (hwtcon->type != TCON_LCD)
			continue;
		if (!hwtcon->is_assigned)
			continue;
		if (hwtcon->conn_type_id == lcd_id)
			break;
	}

	if (i >= tcon_cnt) {
		DRM_ERROR("can NOT get a attached tcon\n");
		return -1;
	}

	return i;
}

#ifdef CONFIG_AW_FPGA_S4
static struct lcd_clk_info clk_tbl[] = {
	{LCD_IF_HV,    0x12, 1, 1, 0},
	{LCD_IF_CPU,   12, 1, 1, 0},
	{LCD_IF_LVDS,   7, 1, 1, 0},
	{LCD_IF_DSI,    4, 1, 4, 0},
};
#else
static struct lcd_clk_info clk_tbl[] = {
	{LCD_IF_HV, 6, 1, 1, 0},
	{LCD_IF_CPU, 12, 1, 1, 0},
	{LCD_IF_LVDS, 7, 1, 1, 0},
#if defined (DSI_VERSION_40)
	{LCD_IF_DSI, 4, 1, 4, 150000000},
#else
	{LCD_IF_DSI, 4, 1, 4, 0},
#endif /* endif DSI_VERSION_40 */
	{LCD_IF_VDPO, 4, 1, 1, 0},
};
#endif

int sunxi_tcon_get_clk_info(int lcd_id, struct lcd_clk_info *info)
{
	int tcon_div = 6;
	int lcd_div = 1;
	int dsi_div = 4;
	int dsi_rate = 0;
	int i;
	int find = 0;
	struct disp_panel_para *panel = sunxi_lcd_get_panel_para(lcd_id);

	if (panel == NULL) {
		DRM_ERROR("panel is NULL\n");
		return 1;
	}

	for (i = 0; i < sizeof(clk_tbl) / sizeof(clk_tbl[0]); i++) {
		if (clk_tbl[i].lcd_if == panel->lcd_if) {
			tcon_div = clk_tbl[i].tcon_div;
			lcd_div = clk_tbl[i].lcd_div;
			dsi_div = clk_tbl[i].dsi_div;
			dsi_rate = clk_tbl[i].dsi_rate;
			find = 1;
			break;
		}
	}
	if (find == 0) {
		DRM_ERROR("cant find clk info for lcd_if %d\n", panel->lcd_if);
		return 1;
	}

#if defined(DSI_VERSION_40)
	if (panel->lcd_if == LCD_IF_DSI) {
		u32 lane = panel->lcd_dsi_lane;
		u32 bitwidth = 0;

		switch (panel->lcd_dsi_format) {
		case LCD_DSI_FORMAT_RGB888:
			bitwidth = 24;
			break;
		case LCD_DSI_FORMAT_RGB666:
			bitwidth = 24;
			break;
		case LCD_DSI_FORMAT_RGB565:
			bitwidth = 16;
			break;
		case LCD_DSI_FORMAT_RGB666P:
			bitwidth = 18;
			break;
		}

		dsi_div = bitwidth / lane;
		if (panel->lcd_dsi_if == LCD_DSI_IF_COMMAND_MODE) {
			tcon_div = dsi_div;
		}
	}
#endif

	if (panel->lcd_if == LCD_IF_HV &&
	    panel->lcd_hv_if == LCD_HV_IF_CCIR656_2CYC &&
	    panel->ccir_clk_div > 0)
		tcon_div = panel->ccir_clk_div;
	else if (panel->lcd_tcon_mode == DISP_TCON_DUAL_DSI &&
		 panel->lcd_if == LCD_IF_DSI) {
		tcon_div = tcon_div / 2;
		dsi_div /= 2;
	}

#if defined(DSI_VERSION_28)
	if (panel->lcd_if == LCD_IF_DSI &&
	    panel->lcd_dsi_if == LCD_DSI_IF_COMMAND_MODE) {
		tcon_div = 6;
		dsi_div = 6;
	}
#endif

	info->tcon_div = tcon_div;
	info->lcd_div = lcd_div;
	info->dsi_div = dsi_div;
	info->dsi_rate = dsi_rate;

	return 0;
}

static int sunxi_tcon_set_clk(unsigned int enc_id, unsigned int lcd_id)
{
	int ret = 0;
	unsigned long pll_rate = 297000000, lcd_rate = 33000000;
	unsigned long dclk_rate = 33000000;
	unsigned long pll_rate_set = 297000000, lcd_rate_set = 33000000;
	struct clk *parent_clk;
	struct sunxi_tcon *hwtcon = sunxi_tcon_get_tcon(enc_id);
	struct lcd_clk_info clk_info;
	struct disp_panel_para *p_panel = NULL;

	if (!hwtcon->mclk) {
		DRM_ERROR("TCON%d has NO clk\n", enc_id);
		return -1;
	}

	if (hwtcon->rst_bus_tcon) {
		ret = reset_control_deassert(hwtcon->rst_bus_tcon);
		if (ret) {
			DRM_ERROR("reset_control_deassert for rst_bus_tcon failed!\n");
			return -1;
		}
	}

	p_panel = sunxi_lcd_get_panel_para(lcd_id);
	memset(&clk_info, 0, sizeof(clk_info));

	ret = sunxi_tcon_get_clk_info(lcd_id, &clk_info);
	if (ret) {
		DRM_WARN("Get clk_info fail!\n");
		return ret;
	}
	dclk_rate = ((unsigned long long)sunxi_lcd_get_dclk(lcd_id)) * 1000000;

	if (p_panel->lcd_if == LCD_IF_DSI) {
		lcd_rate = dclk_rate * clk_info.dsi_div;
		pll_rate = lcd_rate * clk_info.lcd_div;
	} else {
		lcd_rate = dclk_rate * clk_info.tcon_div;
		pll_rate = lcd_rate * clk_info.lcd_div;
	}

	parent_clk = clk_get_parent(hwtcon->mclk);
	if (parent_clk) {
		clk_set_rate(parent_clk, pll_rate);
		pll_rate_set = clk_get_rate(parent_clk);
	}

	if (clk_info.lcd_div)
		lcd_rate_set = pll_rate_set / clk_info.lcd_div;
	else
		lcd_rate_set = pll_rate_set;

	clk_set_rate(hwtcon->mclk, lcd_rate_set);
	lcd_rate_set = clk_get_rate(hwtcon->mclk);

	if (lcd_rate_set != lcd_rate)
		DRM_WARN("Rate to be set:%lu, real clk rate:%lu\n", lcd_rate,
			 lcd_rate_set);

	ret = clk_prepare_enable(hwtcon->mclk);
	if (ret != 0) {
		DRM_ERROR("fail enable TCON%d's clock!\n", enc_id);
		return -1;
	}

	ret = clk_prepare_enable(hwtcon->mclk_bus);
	if (ret != 0) {
		DRM_ERROR("fail enable TCON%d's bus clock!\n", enc_id);
		return -1;
	}

	hwtcon->tcon_div = clk_info.tcon_div;

	return 0;
}

/*
 * referred from sunxi display
 */
static void sunxi_tcon_lcd_calc_judge_line(int enc_id, int lcd_id)
{
	unsigned int usec_per_line, start_delay;
	unsigned int usec_start_delay, usec_judge_point;
	struct sunxi_tcon *hwtcon = sunxi_tcon_get_tcon(enc_id);

	usec_per_line = sunxi_lcd_get_usec_per_line(lcd_id);
	start_delay = tcon_get_start_delay(enc_id, hwtcon->type);
	usec_start_delay = start_delay * usec_per_line;

	if (usec_start_delay <= 200)
		usec_judge_point = usec_start_delay * 3 / 7;
	else if (usec_start_delay <= 400)
		usec_judge_point = usec_start_delay / 2;
	else
		usec_judge_point = 200;

	hwtcon->judge_line = usec_judge_point / usec_per_line;

	DRM_INFO("[SUNXI-TCON]tcon%d judge_line:%u\n",
			enc_id, hwtcon->judge_line);
}



static int sunxi_tcon_lcd_prepare(unsigned int enc_id, unsigned int lcd_id)
{
	int ret = 0;
	struct sunxi_tcon *hwtcon = sunxi_tcon_get_tcon(enc_id);
#if defined(HAVE_DEVICE_COMMON_MODULE)
	struct sunxi_top_if *topif = &tcon_drv->hwtopif;
#endif

	hwtcon->is_assigned = true;
#if defined(HAVE_DEVICE_COMMON_MODULE)
	if (topif->rst_bus_if_top) {
		ret = reset_control_deassert(topif->rst_bus_if_top);
		if (ret) {
			DRM_ERROR("reset_control_deassert for rst_bus_if_top failed!\n");
			return -1;
		}
	}

	if (topif->mclk) {
		ret = clk_prepare_enable(topif->mclk);
		if (ret != 0) {
			DRM_ERROR("fail enable topif's clock!\n");
			return -1;
		}
	}
#endif

	ret = sunxi_tcon_set_clk(enc_id, lcd_id);
	if (ret < 0) {
		DRM_ERROR("sunxi_tcon_set_clk failed\n");
		return -1;
	}

	return 0;
}

static void sunxi_tcon_lcd_unprepare(unsigned int enc_id)
{
	struct sunxi_tcon *hwtcon = sunxi_tcon_get_tcon(enc_id);
#if defined(HAVE_DEVICE_COMMON_MODULE)
	struct sunxi_top_if *topif = &tcon_drv->hwtopif;
#endif

	clk_disable_unprepare(hwtcon->mclk);
#if defined(HAVE_DEVICE_COMMON_MODULE)
	clk_disable_unprepare(topif->mclk);
#endif
	hwtcon->is_assigned = false;
}

static int sunxi_tcon_lcd_sw_set(unsigned int enc_id, unsigned int lcd_id)
{
	int ret = 0;
#if defined(HAVE_DEVICE_COMMON_MODULE)
	struct sunxi_top_if *topif = &tcon_drv->hwtopif;
#endif
	struct sunxi_tcon *hwtcon = sunxi_tcon_get_tcon(enc_id);

	hwtcon->is_assigned = true;
#if defined(HAVE_DEVICE_COMMON_MODULE)
	if (topif->mclk) {
		ret = clk_prepare_enable(topif->mclk);
		if (ret != 0) {
			DRM_ERROR("fail enable topif's clock!\n");
			return -1;
		}
	}
#endif

	ret = clk_prepare_enable(hwtcon->mclk);
	if (ret != 0) {
		DRM_ERROR("fail enable TCON%d's clock!\n", enc_id);
		return -1;
	}

	hwtcon->is_enabled = true;

	return 0;
}

static int
sunxi_tcon_set_lcd(unsigned int de_id, unsigned int enc_id, unsigned int lcd_id,
			struct disp_video_timings *video_info,
			struct sunxi_connector_work_mode *conn_work_mode)
{
	int ret = 0;
	struct sunxi_tcon *hwtcon = sunxi_tcon_get_tcon(enc_id);

	ret = sunxi_tcon_lcd_prepare(enc_id, lcd_id);
	if (ret < 0) {
		DRM_ERROR("sunxi_tcon_lcd_prepare failed\n");
		return ret;
	}

	hwtcon->conn_type_id = lcd_id;
	tcon_init(enc_id);
	tcon0_set_dclk_div(enc_id, hwtcon->tcon_div);
#if IS_ENABLED(CONFIG_ARCH_SUN50IW9)
	if (tcon0_cfg(enc_id, sunxi_lcd_get_panel_para(lcd_id), 0) != 0) {
		DRM_ERROR("lcd cfg fail!\n");
		return -1;
	}
#else
	if (tcon0_cfg(enc_id, sunxi_lcd_get_panel_para(lcd_id)) != 0) {
		DRM_ERROR("lcd cfg fail!\n");
		return -1;
	}
#endif
	tcon0_cfg_ext(enc_id, sunxi_lcd_get_panel_ext_para(lcd_id));

	tcon0_src_select(enc_id, LCD_SRC_DE, de_id);

	sunxi_tcon_lcd_calc_judge_line(enc_id, lcd_id);

	hwtcon->is_enabled = true;

	return 0;
}

static void sunxi_tcon_unset_lcd(unsigned int enc_id)
{
	struct sunxi_tcon *hwtcon = sunxi_tcon_get_tcon(enc_id);

	hwtcon->is_enabled = false;
	hwtcon->conn_type_id = 0;
	hwtcon->judge_line = 0;

	tcon_exit(enc_id);
	sunxi_tcon_lcd_unprepare(enc_id);

	return;
}
#endif


#if defined(CONFIG_AW_DRM_TV) \
	|| defined(CONFIG_AW_DRM_HDMI14) \
	|| defined(CONFIG_AW_DRM_HDMI20)
static int sunxi_tcon_set_clk_rate(struct clk *mclk,
		  struct disp_video_timings *timing,
	struct sunxi_connector_work_mode *conn_work_mode)
{
	struct clk *parent_clk;
	unsigned long rate = 0, round_rate = 0;
	long rate_diff = 0;
	unsigned long parent_rate = 0, parent_round_rate = 0;
	long parent_rate_diff = 0;
	unsigned int div = 1;

	parent_clk = clk_get_parent(mclk);
	if (!parent_clk) {
		DRM_ERROR("can not get tcon parent clk!\n");
		return -1;
	}

	/* calculate rate */
	rate = timing->pixel_clk * (timing->pixel_repeat + 1);
	if (conn_work_mode->color_fmt == DISP_CSC_TYPE_YUV420)
		rate /= 2;

	round_rate = clk_round_rate(mclk, rate);
	rate_diff = (long)(round_rate - rate);
	if ((rate_diff > 5000000) || (rate_diff < -5000000)) {
		for (div = 1; (rate * div) <= 600000000; div++) {
			parent_rate = rate * div;
			parent_round_rate = clk_round_rate(parent_clk,
							   parent_rate);
			parent_rate_diff = (long)(parent_round_rate - parent_rate);
			if ((parent_rate_diff < 5000000)
				&& (parent_rate_diff > -5000000)) {
				clk_set_rate(parent_clk, parent_rate);
				clk_set_rate(mclk, rate);
				break;
			}
		}

		if ((rate * div) > 600000000)
			clk_set_rate(mclk, rate);
	} else {
		clk_set_rate(mclk, rate);
	}

	return 0;

}

static bool sunxi_tcon_tv_is_support(int nr, int tv_id)
{
	return sunxi_tcon_type_is_support(nr, DISP_OUTPUT_TYPE_TV);
}

/**
 * @name       :sunxi_tcon_tv_prepare
 * @brief      :config and enable tcon's clks
 * @param[IN]  :enc_id:index of tcon(encoder)
 * @param[IN]  :p_info:video timing of current resolution
 * @return     :0 if success, -1 else
 */
static int sunxi_tcon_tv_prepare(unsigned int enc_id,
			struct disp_video_timings *p_info,
		struct sunxi_connector_work_mode *conn_work_mode)
{
	int ret = 0;
	struct sunxi_tcon *hwtcon = sunxi_tcon_get_tcon(enc_id);
#if defined(HAVE_DEVICE_COMMON_MODULE)
	struct sunxi_top_if *topif = &tcon_drv->hwtopif;

	if (!topif) {
		DRM_ERROR("Null pointer: topif\n");
		return -1;
	}
#endif

	if (!hwtcon || !p_info) {
		DRM_ERROR("Null pointer: hwtcon:%p p_info:%p\n",
			hwtcon, p_info);
		return -1;
	}

	hwtcon->is_assigned = true;
#if defined(HAVE_DEVICE_COMMON_MODULE)
	if (topif->mclk) {
		ret = clk_prepare_enable(topif->mclk);
		if (ret != 0) {
			DRM_ERROR("fail enable topif's clock!\n");
			return -1;
		}
	}
#endif

	sunxi_tcon_set_clk_rate(hwtcon->mclk, p_info, conn_work_mode);
	/* clk_set_rate(hwtcon->mclk, p_info->pixel_clk); */

	ret = clk_prepare_enable(hwtcon->mclk);
	if (ret != 0) {
		DRM_ERROR("fail enable tcon's clock!\n");
		return -1;
	}

	return 0;
}

static void sunxi_tcon_tv_unprepare(unsigned int enc_id)
{
	struct sunxi_tcon *hwtcon = sunxi_tcon_get_tcon(enc_id);
#if defined(HAVE_DEVICE_COMMON_MODULE)
	struct sunxi_top_if *topif = &tcon_drv->hwtopif;

	if (!topif) {
		DRM_ERROR("Null pointer!\n");
		return;
	}
#endif

	hwtcon->is_assigned = false;
	clk_disable_unprepare(hwtcon->mclk);
#if defined(HAVE_DEVICE_COMMON_MODULE)
	clk_disable_unprepare(topif->mclk);
#endif
}

static int sunxi_tcon_sw_set_tv(unsigned int enc_id, unsigned int lcd_id)
{
	int ret = 0;
	struct sunxi_tcon *hwtcon = sunxi_tcon_get_tcon(enc_id);
#if defined(HAVE_DEVICE_COMMON_MODULE)
	struct sunxi_top_if *topif = &tcon_drv->hwtopif;

	if (!topif) {
		DRM_ERROR("Null pointer!\n");
		return -1;
	}
#endif

	if (!hwtcon) {
		DRM_ERROR("Null pointer!\n");
		return -1;
	}

	hwtcon->is_assigned = true;
#if defined(HAVE_DEVICE_COMMON_MODULE)
	if (topif->mclk) {
		ret = clk_prepare_enable(topif->mclk);
		if (ret != 0) {
			DRM_ERROR("fail enable topif's clock!\n");
			return -1;
		}
	}
#endif

	ret = clk_prepare_enable(hwtcon->mclk);
	if (ret != 0) {
		DRM_ERROR("fail enable tcon's clock!\n");
		return -1;
	}

	return 0;
}

/**
 * @name       :sunxi_tcon_unset_tv
 * @brief      :disable specified tcon_tv
 * @param[IN]  :enc_id:index of tcon
 * @return     :NONE
 */
static void sunxi_tcon_unset_tv(unsigned int enc_id)
{
	struct sunxi_tcon *hwtcon = sunxi_tcon_get_tcon(enc_id);

	hwtcon->is_enabled = false;
	hwtcon->conn_type_id = 0;
	hwtcon->judge_line = 0;
	tcon1_close(enc_id);
	tcon_irq_disable(enc_id, LCD_IRQ_TCON1_VBLK);
	tcon_exit(enc_id);
	tcon1_hdmi_clk_enable(enc_id, 0);

	sunxi_tcon_tv_unprepare(enc_id);
}
#endif

#if defined(CONFIG_AW_DRM_TV)
/**
 * @name       :sunxi_tcon_set_tv
 * @brief      :set tcon_tv according disp_video_timings
 * @param[IN]  :enc_id: index of tcon
 * @param[IN]  :tv_id: index of tv
 * @param[IN]  :de_id: index of de
 * @param[IN]  :video_info:pointer of video timing to be set
 * @param[IN]  :tv_type: see definition of disp_tv_output
 * @return     :always 0
 */
static int
sunxi_tcon_set_tv(unsigned int de_id, unsigned int enc_id, unsigned int tv_id,
		struct disp_video_timings *video_info,
		struct sunxi_connector_work_mode *conn_work_mode)
{
	int ret;
	struct sunxi_tcon *hwtcon = sunxi_tcon_get_tcon(enc_id);
	enum disp_tv_output tv_type = sunxi_tv_get_interface_type(tv_id);

	ret = sunxi_tcon_tv_prepare(enc_id, video_info, conn_work_mode);
	if (ret < 0) {
		DRM_ERROR("sunxi_tcon_tv_prepare failed\n");
		return ret;
	}

	hwtcon->conn_type_id = tv_id;

	tcon_init(enc_id);
	tcon1_set_timming(enc_id, video_info);
	if (tv_type != DISP_VGA)
		tcon1_yuv_range(enc_id, 1);
	tcon1_src_select(enc_id, LCD_SRC_DE, de_id);

	hwtcon->judge_line = 4;

	hwtcon->is_enabled = true;

	tcon1_tv_clk_enable(enc_id, 1);
	tcon1_open(enc_id);
	if (tv_type == DISP_VGA)
		tcon1_out_to_gpio(enc_id);

	return 0;
}

#elif defined(CONFIG_AW_DRM_HDMI14) || defined(CONFIG_AW_DRM_HDMI20)
/**
 * @name       :sunxi_tcon_set_hdmi
 * @brief      :set tcon_hdmi according disp_video_timings
 * @param[IN]  :enc_id: index of tcon
 * @param[IN]  :hdmi_id: index of hdmi
 * @param[IN]  :de_id: index of de
 * @param[IN]  :video_info:pointer of video timing to be set
 * @return     :always 0
 */
static int
sunxi_tcon_set_hdmi(unsigned int de_id, unsigned int enc_id, unsigned int hdmi_id,
		     struct disp_video_timings *video_info,
		     struct sunxi_connector_work_mode *conn_work_mode)
{
	int ret, fps;

	struct sunxi_tcon *hwtcon = sunxi_tcon_get_tcon(enc_id);

	ret = sunxi_tcon_tv_prepare(enc_id, video_info, conn_work_mode);
	if (ret < 0) {
		DRM_ERROR("sunxi_tcon_tv_prepare failed\n");
		return ret;
	}

	fps = video_info->pixel_clk / video_info->hor_total_time
			/video_info->ver_total_time
			* (video_info->b_interlace + 1)
			/ (video_info->trd_mode + 1);
	tcon_init(enc_id);
	tcon1_set_timming(enc_id, video_info);

/* tcon pol issue */
#if defined(DISP2_TCON_TV_SYNC_POL_ISSUE)
	tcon_set_sync_pol(de_id, !video_info->ver_sync_polarity,
			!video_info->hor_sync_polarity);
#endif

	/*
	* If yuv output(cs != 0), remap yuv plane to (v y u) sequency
	* else disable color remap function
	*/
	if (conn_work_mode->color_fmt == DISP_CSC_TYPE_RGB)
		tcon1_hdmi_color_remap(de_id, 0);
	else
		tcon1_hdmi_color_remap(de_id, 1);

	tcon1_src_select(enc_id, LCD_SRC_DE, de_id);
	tcon1_black_src(de_id, 0, conn_work_mode->color_fmt);

	hwtcon->judge_line = 4;

	hwtcon->is_enabled = true;

	tcon1_hdmi_clk_enable(enc_id, 1);

	tcon1_open(enc_id);

	return 0;
}

#endif

/*
unsigned int sunxi_tcon_get_tcon_type(unsigned int nr)
{
	struct sunxi_tcon *hwtcon = sunxi_tcon_get_tcon(nr);

	return hwtcon->type;
}

unsigned int sunxi_tcon_get_judge_line(unsigned int nr)
{
	struct sunxi_tcon *hwtcon = sunxi_tcon_get_tcon(nr);

	return hwtcon->judge_line;
}

unsigned int sunxi_tcon_get_cur_line(unsigned int nr)
{
	struct sunxi_tcon *hwtcon = sunxi_tcon_get_tcon(nr);

	return tcon_get_cur_line(nr, hwtcon->type);
}

unsigned int sunxi_tcon_get_start_delay(unsigned int nr)
{
	struct sunxi_tcon *hwtcon = sunxi_tcon_get_tcon(nr);

	return tcon_get_start_delay(nr, hwtcon->type);
}
 */

/*
 * referred from sunxi display
 */
bool sunxi_tcon_sync_time_is_enough(unsigned int nr)
{
	int cur_line, judge_line, start_delay;
	unsigned int tcon_type;
	struct sunxi_tcon *hwtcon = sunxi_tcon_get_tcon(nr);

	tcon_type = hwtcon->type;
	judge_line = hwtcon->judge_line;

	cur_line = tcon_get_cur_line(nr, tcon_type);
	start_delay = tcon_get_start_delay(nr, tcon_type);

	/*
	DRM_INFO("cur_line:%d start_delay:%d judge_line:%d\n",
			cur_line, start_delay, judge_line);
	 */

	if (cur_line <= (start_delay - judge_line))
		return true;

	return false;
}

static int sunxi_tcon_init_al(void)
{
	int i;
	struct disp_bsp_init_para *para;

	para = sunxi_disp_get_bsp_init_para();
	if (!para) {
		DRM_ERROR("sunxi_disp_get_bsp_init_para failed\n");
		return -1;
	}

	for (i = 0; i < tcon_drv->tcon_cnt; i++) {
		para->reg_base[DISP_MOD_LCD0 + i]
				= tcon_drv->hwtcon[i].reg_base;
		tcon_set_reg_base(i, tcon_drv->hwtcon[i].reg_base);
	}

#if defined(HAVE_DEVICE_COMMON_MODULE)
	para->reg_base[DISP_MOD_DEVICE] = tcon_drv->hwtopif.reg_base;
	tcon_top_set_reg_base(0, para->reg_base[DISP_MOD_DEVICE]);
#endif

	return 0;
}

static struct sunxi_tcon_funcs lcd_tcon = {
	.is_use_irq = sunxi_tcon_is_use_irq,
	.get_irq_no = sunxi_tcon_get_irq_no,

#ifdef CONFIG_AW_DRM_LCD
	.conn_is_support = sunxi_tcon_lcd_is_support,
	.set = sunxi_tcon_set_lcd,
	.sw_set = sunxi_tcon_lcd_sw_set,
	.unset = sunxi_tcon_unset_lcd,
#endif
};


static struct sunxi_tcon_funcs tv_tcon = {
	.is_use_irq = sunxi_tcon_is_use_irq,
	.get_irq_no = sunxi_tcon_get_irq_no,
#ifdef CONFIG_AW_DRM_TV
	.conn_is_support = sunxi_tcon_tv_is_support,
	.set = sunxi_tcon_set_tv,
	.sw_set = sunxi_tcon_sw_set_tv,
	.unset = sunxi_tcon_unset_tv,
#endif
};

static struct sunxi_tcon_funcs hdmi_tcon = {
	.is_use_irq = sunxi_tcon_is_use_irq,
	.get_irq_no = sunxi_tcon_get_irq_no,
#if defined(CONFIG_AW_DRM_HDMI14) || defined(CONFIG_AW_DRM_HDMI20)
	.conn_is_support = sunxi_tcon_tv_is_support,
	.set = sunxi_tcon_set_hdmi,
	.sw_set = sunxi_tcon_sw_set_tv,
	.unset = sunxi_tcon_unset_tv,
#endif
};

struct sunxi_tcon_funcs *
sunxi_tcon_attach_connector_type(int enc_id, int conn_type)
{
	struct sunxi_tcon *hwtcon = sunxi_tcon_get_tcon(enc_id);

	if (conn_type == DISP_OUTPUT_TYPE_LCD) {
		hwtcon->funcs = &lcd_tcon;
	} else if (conn_type == DISP_OUTPUT_TYPE_TV) {
		hwtcon->funcs = &tv_tcon;
	} else if (conn_type == DISP_OUTPUT_TYPE_HDMI) {
		hwtcon->funcs = &hdmi_tcon;
	} else {
		hwtcon->funcs = NULL;
		DRM_ERROR("NOT support connector type:%d\n", conn_type);
	}

	return hwtcon->funcs;
}

void sunxi_tcon_unattach_connector_type(int enc_id)
{
	struct sunxi_tcon *hwtcon = sunxi_tcon_get_tcon(enc_id);

	hwtcon->funcs = NULL;
}

static int sunxi_tcon_parse_dts(struct platform_device *pdev)
{
	struct device_node *node;
	struct sunxi_tcon *hwtcon;
	int i = 0, reg_index = 0, clk_index = 0;
	char id[32];
	char id2[32];
	char id3[32];

	node = pdev->dev.of_node;
	if (!node) {
		DRM_ERROR("get sunxi-tcon node err.\n ");
		return -EINVAL;
	}

/* parse disp_if_top */
	reg_index = 0;
	clk_index = 0;
#if defined(HAVE_DEVICE_COMMON_MODULE)
	tcon_drv->hwtopif.reg_base =
			(uintptr_t __force)of_iomap(node, reg_index++);
	if (!tcon_drv->hwtopif.reg_base) {
		DRM_ERROR("unable to map device common module registers\n");
		return -EINVAL;
	}
	DRM_DEBUG_DRIVER("[SUNXI-TCON]get disp_if_top reg_base:0x%lx\n",
			tcon_drv->hwtopif.reg_base);

	tcon_drv->hwtopif.mclk = devm_clk_get(&pdev->dev, "clk_bus_dpss_top0");
	if (IS_ERR(tcon_drv->hwtopif.mclk)) {
		DRM_ERROR("fail to get clk for device common module\n");
		return -EINVAL;
	}

	tcon_drv->hwtopif.rst_bus_if_top = devm_reset_control_get_shared(&pdev->dev, "rst_bus_dpss_top");
	if (IS_ERR(tcon_drv->hwtopif.rst_bus_if_top)) {
		DRM_ERROR("fail to get reset clk for rst_bus_dpss_top\n");
		return -EINVAL;
	}
#endif

	for (i = 0; i < tcon_drv->tcon_cnt; i++) {
		sprintf(id, "clk_tcon%d", i);
		sprintf(id2, "clk_bus_tcon%d", i);
		sprintf(id3, "rst_bus_tcon%d", i);

		hwtcon = &tcon_drv->hwtcon[i];

		hwtcon->reg_base
			= (uintptr_t __force)of_iomap(node, reg_index);
		if (!hwtcon->reg_base) {
			DRM_ERROR("unable to map tcon:%d registers\n", i);
			return -1;
		}

		hwtcon->irq_no = irq_of_parse_and_map(node, i);
		if (!hwtcon->irq_no) {
			DRM_ERROR("get irq no of tcon%d failed\n", i);
			return -1;
		}
		DRM_DEBUG_DRIVER("[SUNXI-TCON]TCON%d get irq_no:%d\n",
						   i, hwtcon->irq_no);

		hwtcon->mclk = devm_clk_get(&pdev->dev, id);
		if (IS_ERR(hwtcon->mclk)) {
			DRM_ERROR("fail to get clk for tcon %d\n", i);
			return -1;
		}

		hwtcon->mclk_bus = devm_clk_get(&pdev->dev, id2);
		if (IS_ERR(hwtcon->mclk_bus)) {
			DRM_ERROR("fail to get clk bus for tcon %d\n", i);
			return -1;
		}
		DRM_DEBUG_DRIVER("[SUNXI-TCON] %s, %s, %x, %x\n", id, id2, hwtcon->mclk, hwtcon->mclk_bus);

		hwtcon->rst_bus_tcon = devm_reset_control_get_shared(&pdev->dev, id3);
		if (IS_ERR(hwtcon->rst_bus_tcon)) {
			DRM_ERROR("fail to get reset clk for %s\n", id3);
			return -EINVAL;
		}

		reg_index++;
		clk_index++;
	}

	return 0;
}

static int sunxi_tcon_probe(struct platform_device *pdev)
{
	int i;
	struct sunxi_tcon *hwtcon;
	unsigned int output_type; /* type outputdata from tcon and connector */

	DRM_INFO("[SUNXI-TCON] sunxi_tcon_probe start\n");

	tcon_drv->pdev = pdev;

	tcon_drv->tcon_cnt = de_feat_get_num_devices();
	if ((tcon_drv->tcon_cnt <= 0) || (tcon_drv->tcon_cnt > TCON_NUM_MAX)) {
		DRM_ERROR("get wrong tcon count:%d\n",
						tcon_drv->tcon_cnt);
		goto tcon_err;
	}

	for (i = 0; i < tcon_drv->tcon_cnt; i++) {
		hwtcon = &tcon_drv->hwtcon[i];
		hwtcon->id = i;

		/* judge the type of tcon */
		output_type = de_feat_get_supported_output_types(i);
		if ((output_type & DISP_OUTPUT_TYPE_LCD)
			|| (output_type & DISP_OUTPUT_TYPE_EDP)) {
			tcon_drv->tcon_lcd_cnt++;
			hwtcon->type_id = tcon_drv->tcon_lcd_cnt - 1;
			hwtcon->type = TCON_LCD;
			DRM_DEBUG_DRIVER("[SUNXI-TCON]tcon%d output_type:%s"
							" is TCON_LCD%d\n",
			hwtcon->id, sunxi_tcon_get_output_type_name(output_type),
							hwtcon->type_id);
		} else if ((output_type & DISP_OUTPUT_TYPE_HDMI)
			|| (output_type & DISP_OUTPUT_TYPE_TV)
			|| (output_type & DISP_OUTPUT_TYPE_VGA)) {
			tcon_drv->tcon_tv_cnt++;
			hwtcon->type_id = tcon_drv->tcon_tv_cnt - 1;
			hwtcon->type = TCON_TV;
			DRM_DEBUG_DRIVER("[SUNXI-TCON]tcon%d output_type:%s"
							" is TCON_TV%d\n",
			hwtcon->id, sunxi_tcon_get_output_type_name(output_type),
							hwtcon->type_id);
		} else {
			DRM_ERROR("tcon%d has wrong type:%s\n",
							i,
				sunxi_tcon_get_output_type_name(output_type));
			return -1;
		}
	}

	if (sunxi_tcon_parse_dts(pdev) < 0) {
		DRM_ERROR("sunxi_tcon_parse_dts failed\n");
		goto tcon_err;
	}

	if (sunxi_tcon_init_al() < 0) {
		DRM_ERROR("sunxi_tcon_init_al failed\n");
		goto tcon_err;
	}

	for (i = 0; i < tcon_drv->tcon_cnt; i++) {
		hwtcon = &tcon_drv->hwtcon[i];
		if (hwtcon->type == TCON_TV)
			 sunxi_tcon_pad_sel(i, 1);
	}

	return 0;

tcon_err:
	DRM_ERROR("[SUNXI-DE] sunxi_tcon_probe FAILED\n");
	return -EINVAL;
}

static int sunxi_tcon_remove(struct platform_device *pdev)
{
	return 0;
}

/* Note: sunxi-lcd is represented of sunxi tcon,
 * using lcd is order to be same with sunxi display driver
 */
static const struct of_device_id sunxi_tcon_match[] = {

	{ .compatible = "allwinner,sunxi-tcon", },
	{},
};

struct platform_driver sunxi_tcon_platform_driver = {
	.probe = sunxi_tcon_probe,
	.remove = sunxi_tcon_remove,
	.driver = {
		   .name = "tcon",
		   .owner = THIS_MODULE,
		   .of_match_table = sunxi_tcon_match,
	},
};

int sunxi_tcon_module_init(void)
{
	int ret = 0, err;
	struct drv_model_info  *drv_model;

	DRM_INFO("[SUNXI-TCON]sunxi_tcon_module_init\n");

	tcon_drv = kzalloc(sizeof(*tcon_drv), GFP_KERNEL);
	if (!tcon_drv) {
		DRM_ERROR("can NOT allocate memory for tcon_drv\n");
		goto tcon_err;
	}

	drv_model = &tcon_drv->drv_model;

	if (alloc_chrdev_region(&drv_model->devid, 0, 1, "tcon") < 0) {
		DRM_ERROR("alloc_chrdev_region failed\n");
		goto tcon_err;
	}

	drv_model->cdev = cdev_alloc();
	if (!drv_model->cdev) {
		DRM_ERROR("cdev_alloc failed\n");
		goto tcon_err;
	}

	cdev_init(drv_model->cdev, NULL);
	drv_model->cdev->owner = THIS_MODULE;
	err = cdev_add(drv_model->cdev, drv_model->devid, 1);
	if (err) {
		DRM_ERROR("cdev_add major number:%d failed\n",
						MAJOR(drv_model->devid));
		goto tcon_err;
	}

	drv_model->sysclass = class_create(THIS_MODULE, "tcon");
	if (IS_ERR(drv_model->sysclass)) {
		DRM_ERROR("create class error\n");
		goto tcon_err;
	}

	drv_model->dev = device_create(drv_model->sysclass, NULL,
					drv_model->devid, NULL, "tcon");
	if (!drv_model->dev) {
		DRM_ERROR("device_create failed\n");
		goto tcon_err;
	}

	ret = platform_driver_register(&sunxi_tcon_platform_driver);
	if (ret) {
		DRM_ERROR("platform_driver_register failed\n");
		goto tcon_err;
	}

	ret = sysfs_create_group(&drv_model->dev->kobj,
					&tcon_attribute_group);
	if (ret < 0) {
		DRM_ERROR("sysfs_create_file fail!\n");
		goto tcon_err;
	}

	DRM_INFO("[SUNXI-TCON]sunxi_tcon_module_init end\n\n");
	return 0;

tcon_err:
	kfree(tcon_drv);
	DRM_ERROR("sunxi_tcon_module_init FAILED\n");
	return -1;
}

void sunxi_tcon_module_exit(void)
{
	struct drv_model_info  *drv_model;

	DRM_INFO("[SUNXI-TCON]sunxi_tcon_module_exit\n");

	drv_model = &tcon_drv->drv_model;
	platform_driver_unregister(&sunxi_tcon_platform_driver);
	device_destroy(drv_model->sysclass, drv_model->devid);
	class_destroy(drv_model->sysclass);

	cdev_del(drv_model->cdev);
	kfree(tcon_drv);
}

