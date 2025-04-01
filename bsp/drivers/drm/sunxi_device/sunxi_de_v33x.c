/* sunxi_de_v33x.c
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
#include <uapi/drm/drm_fourcc.h>
#include <linux/clk.h>
#include <linux/wait.h>
#include <drm/drm_print.h>

#include "sunxi_de.h"

#include "disp_al_de.h"
#include "de330/de_rtmx.h"
#include "de330/de_enhance.h"
#include "de330/de_smbl.h"
#include "de330/de_wb.h"
#include "de330/de_bld.h"
#include "de330/de_ccsc.h"

#include "tcon/de_lcd.h"
#include "tcon/de_dsi.h"

#include "sunxi_common.h"
#include "sunxi_tcon.h"

enum rcq_update {
	RCQ_UPDATE_NONE = 0,
	RCQ_UPDATE_ACCEPT = 1,
	RCQ_UPDATE_FINISHED = 2,
};

#define MY_BYTE_ALIGN(x) (((x + (4*1024-1)) >> 12) << 12)

static atomic_t rcq_update_flag;
static wait_queue_head_t rcq_update_queue;

static struct sunxi_de_drv *de_drv;

#if defined(CONFIG_AW_IOMMU)
#define DE_MASTOR_ID 0
extern void sunxi_enable_device_iommu(unsigned int mastor_id, bool flag);
#endif

static const unsigned int ui_layer_formats[] = {
	DRM_FORMAT_RGB888,
	DRM_FORMAT_BGR888,

	DRM_FORMAT_ARGB8888,
	DRM_FORMAT_ABGR8888,
	DRM_FORMAT_RGBA8888,
	DRM_FORMAT_BGRA8888,

	DRM_FORMAT_XRGB8888,
	DRM_FORMAT_XBGR8888,
	DRM_FORMAT_RGBX8888,
	DRM_FORMAT_BGRX8888,

	DRM_FORMAT_RGB565,
	DRM_FORMAT_BGR565,

	DRM_FORMAT_ARGB4444,
	DRM_FORMAT_ABGR4444,
	DRM_FORMAT_RGBA4444,
	DRM_FORMAT_BGRA4444,

	DRM_FORMAT_ARGB1555,
	DRM_FORMAT_ABGR1555,
	DRM_FORMAT_RGBA5551,
	DRM_FORMAT_BGRA5551,
};

static const unsigned int vi_layer_formats[] = {
	DRM_FORMAT_RGB888,
	DRM_FORMAT_BGR888,

	DRM_FORMAT_ARGB8888,
	DRM_FORMAT_ABGR8888,
	DRM_FORMAT_RGBA8888,
	DRM_FORMAT_BGRA8888,

	DRM_FORMAT_XRGB8888,
	DRM_FORMAT_XBGR8888,
	DRM_FORMAT_RGBX8888,
	DRM_FORMAT_BGRX8888,

	DRM_FORMAT_RGB565,
	DRM_FORMAT_BGR565,

	DRM_FORMAT_ARGB4444,
	DRM_FORMAT_ABGR4444,
	DRM_FORMAT_RGBA4444,
	DRM_FORMAT_BGRA4444,

	DRM_FORMAT_ARGB1555,
	DRM_FORMAT_ABGR1555,
	DRM_FORMAT_RGBA5551,
	DRM_FORMAT_BGRA5551,

	DRM_FORMAT_AYUV,
	DRM_FORMAT_YUV444,
	DRM_FORMAT_YUV422,
	DRM_FORMAT_YUV420,
	DRM_FORMAT_YUV411,

	DRM_FORMAT_NV61,
	DRM_FORMAT_NV16,
	DRM_FORMAT_NV21,
	DRM_FORMAT_NV12,
};

static struct sunxi_de *sunxi_de_get_de(int nr)
{
	return &de_drv->hwde[nr];
}

/* get crtc count */
int sunxi_de_get_count(void)
{
	/* return the DE count */
	return de_drv->de_cnt;
}

struct sunxi_de_funcs *sunxi_de_get_funcs(int nr)
{
	struct sunxi_de *hwde = sunxi_de_get_de(nr);

	return (struct sunxi_de_funcs *)hwde->funcs;
}

int disp_feat_is_using_rcq(unsigned int disp)
{
	return de_feat_is_using_rcq(disp);
}

int disp_feat_is_using_wb_rcq(unsigned int wb)
{
	return de_feat_is_using_wb_rcq(wb);
}

void *disp_malloc(u32 num_bytes, void *phys_addr)
{
	u32 actual_bytes;
	void *address = NULL;

	if (num_bytes != 0) {
		actual_bytes = MY_BYTE_ALIGN(num_bytes);

		address =
		    dma_alloc_coherent(&de_drv->pdev->dev, actual_bytes,
				       (dma_addr_t *) phys_addr, GFP_KERNEL);
		if (address) {
			DRM_INFO
			    ("dma_alloc_coherent ok, address=0x%p, size=0x%x\n",
			     (void *)(*(unsigned long *)phys_addr), num_bytes);
			return address;
		}

		DRM_ERROR("dma_alloc_coherent fail, size=0x%x\n", num_bytes);
		return NULL;
	}

	DRM_ERROR("%s size is zero\n", __func__);

	return NULL;
}

void disp_free(void *virt_addr, void *phys_addr, u32 num_bytes)
{
	u32 actual_bytes;

	actual_bytes = MY_BYTE_ALIGN(num_bytes);
	if (phys_addr && virt_addr)
		dma_free_coherent(&de_drv->pdev->dev, actual_bytes, virt_addr,
				  (dma_addr_t)phys_addr);
}

static unsigned long sunxi_de_get_freq(int nr)
{
	return clk_get_rate(de_drv->mclk);
}

static int sunxi_de_get_layer_count(int nr)
{
	struct sunxi_de *hwde = sunxi_de_get_de(nr);

	return ((hwde->vichannel_cnt * hwde->layers_per_vichannel)
		+ (hwde->uichannel_cnt * hwde->layers_per_uichannel));
}

static int sunxi_de_get_vi_layer_count(int nr)
{
	struct sunxi_de *hwde = sunxi_de_get_de(nr);

	return (hwde->vichannel_cnt * hwde->layers_per_vichannel);
}

static int sunxi_de_get_ui_layer_count(int nr)
{
	struct sunxi_de *hwde = sunxi_de_get_de(nr);

	return (hwde->uichannel_cnt * hwde->layers_per_uichannel);
}

static int sunxi_de_layer_is_video(int nr, int layer_id)
{
	struct sunxi_de *hwde = sunxi_de_get_de(nr);

	if (layer_id >= sunxi_de_get_layer_count(nr)) {
		DRM_ERROR("layer:%d if is out of range\n", layer_id);
		return -1;
	}

	if (layer_id < (hwde->vichannel_cnt * hwde->layers_per_vichannel))
		return 1;
	return 0;
}

static int sunxi_de_get_layer_channel_id(int nr, int layer_id)
{
	struct sunxi_de *hwde = sunxi_de_get_de(nr);

	if (sunxi_de_layer_is_video(nr, layer_id))
		return layer_id / hwde->layers_per_vichannel;

	return hwde->vichannel_cnt +
		((layer_id - sunxi_de_get_vi_layer_count(nr)) / hwde->layers_per_uichannel);
}

static int sunxi_de_get_layer_id_within_chanel(int nr, int top_layer_id)
{
	struct sunxi_de *hwde = sunxi_de_get_de(nr);

	if (top_layer_id < sunxi_de_get_vi_layer_count(nr))
		return top_layer_id % hwde->layers_per_vichannel;

	return top_layer_id % hwde->layers_per_uichannel;
}

static int sunxi_de_get_layer_formats(int nr, unsigned int layer_id,
				const unsigned int **formats,
				unsigned int *count)
{
	int is_video;

	is_video = sunxi_de_layer_is_video(nr, layer_id);
	if (is_video < 0) {
		DRM_ERROR("judge layer_is_video failed\n");
		return -1;
	}

	if (is_video) {
		*formats = vi_layer_formats;
		*count = ARRAY_SIZE(vi_layer_formats);
	} else {
		*formats = ui_layer_formats;
		*count = ARRAY_SIZE(ui_layer_formats);
	}

	return 0;
}

static bool sunxi_de_is_support_tcon(int nr, int tcon_id)
{
	return true;
}

static int sunxi_de_get_attached_tcon_id(int nr)
{
	struct sunxi_de *hwde = sunxi_de_get_de(nr);

	if (!hwde->enable) {
		DRM_ERROR("DE%d is NOT be set to enable\n", hwde->id);
		return -1;
	}

	return hwde->info.hwdev_index;
}

static bool sunxi_de_is_enable(int nr)
{
	struct sunxi_de *hwde = sunxi_de_get_de(nr);

	return hwde->enable;
}

static int sunxi_de_protect_reg_for_rcq(
		struct sunxi_de *de, bool protect)
{
	int ret;

	if (protect && (atomic_read(&rcq_update_flag) == RCQ_UPDATE_ACCEPT)) {
		ret = wait_event_timeout(rcq_update_queue,
				atomic_read(&rcq_update_flag) == RCQ_UPDATE_FINISHED,
				msecs_to_jiffies(1000 / de->info.device_fps + 1));
		if (ret <= 0) {
			DRM_ERROR("Wait rcq timeout!skip frame!\n");
			atomic_set(&rcq_update_flag, RCQ_UPDATE_FINISHED);
			wake_up(&rcq_update_queue);
			de_rtmx_set_rcq_update(de->id, 0);
		} else
			de_rtmx_set_all_rcq_head_dirty(de->id, 0);

	} else if (!protect) {
		de_rtmx_set_all_rcq_head_dirty(de->id, 1);
		de_rtmx_set_rcq_update(de->id, 1);
		atomic_set(&rcq_update_flag, RCQ_UPDATE_ACCEPT);
	}

	return 0;
}

static int sunxi_de_layer_enhance_apply(unsigned int disp,
	struct disp_layer_config_data *data,
	unsigned int layer_num)
{
	struct disp_enhance_chn_info *ehs_info;
	struct de_rtmx_context *ctx = de_rtmx_get_context(disp);
	struct de_chn_info *chn_info = ctx->chn_info;
	u32 vi_chn_num = de_feat_get_num_vi_chns(disp);
	u32 i;

	ehs_info = kmalloc(sizeof(*ehs_info) * vi_chn_num,
			GFP_KERNEL | __GFP_ZERO);
	if (ehs_info == NULL) {
		pr_warn("%s failed to kmalloc!\n", __func__);
		return -1;
	}
	memset((void *)ehs_info, 0, sizeof(*ehs_info)
			* vi_chn_num);
	for (i = 0; i < layer_num; ++i, ++data) {
		if (data->config.enable
			&& (data->config.channel < vi_chn_num)) {
			struct disp_enhance_layer_info *ehs_layer_info =
					&ehs_info[data->config.channel]
					.layer_info[data->config.layer_id];

			ehs_layer_info->fb_size.width =
				data->config.info.fb.size[0].width;
			ehs_layer_info->fb_size.height =
				data->config.info.fb.size[0].height;
			ehs_layer_info->fb_crop.x =
				data->config.info.fb.crop.x >> 32;
			ehs_layer_info->fb_crop.y =
				data->config.info.fb.crop.y >> 32;
			ehs_layer_info->en = 1;
			ehs_layer_info->format = data->config.info.fb.format;
		}
	}
	for (i = 0; i < vi_chn_num; i++) {
		ehs_info[i].ovl_size.width = chn_info->ovl_out_win.width;
		ehs_info[i].ovl_size.height = chn_info->ovl_out_win.height;
		ehs_info[i].bld_size.width = chn_info->scn_win.width;
		ehs_info[i].bld_size.height = chn_info->scn_win.height;

	}
	/* set enhance size */
	de_enhance_layer_apply(disp, ehs_info);
	kfree(ehs_info);
	return 0;
}

static int sunxi_de_single_layer_apply(
	int nr, struct disp_layer_config_data *data)
{
	int layer_id;
	struct disp_layer_config_data *data_tmp = NULL;
	struct sunxi_de *hwde = sunxi_de_get_de(nr);

	layer_id = data->config.channel * 4 + data->config.layer_id;
	data_tmp = &hwde->layer_config_data[layer_id];
	memcpy(data_tmp, data,
		sizeof(*data));

	if (hwde->use_rcq)
		sunxi_de_protect_reg_for_rcq(hwde, true);

	de_rtmx_layer_apply(nr, hwde->layer_config_data,
		sunxi_de_get_layer_count(nr));

	sunxi_de_layer_enhance_apply(nr, hwde->layer_config_data,
		sunxi_de_get_layer_count(nr));

	de_rtmx_update_reg_ahb(nr);

	if (hwde->use_rcq) {
		sunxi_de_protect_reg_for_rcq(hwde, false);
		de_top_query_state_with_clear(nr, DE_IRQ_STATE_MASK);
	}

	hwde->layer_enabled[data->config.channel][data->config.layer_id]
		= data->config.enable;

	return 0;
}

int sunxi_de_multi_layers_apply(int nr, struct disp_layer_config_data *data,
					unsigned int layer_num)
{
	int i, layer_id;
	struct disp_layer_config_data *data_tmp = NULL;
	struct sunxi_de *hwde = sunxi_de_get_de(nr);

	for (i = 0; i < layer_num; i++) {
		layer_id = data[i].config.channel * 4 + data[i].config.layer_id;
		data_tmp = &hwde->layer_config_data[layer_id];
		memcpy(data_tmp, &data[i],
			sizeof(*data));
		hwde->layer_enabled[data[i].config.channel][data[i].config.layer_id]
			= data[i].config.enable;
	}

	if (hwde->use_rcq)
		sunxi_de_protect_reg_for_rcq(hwde, true);

	de_rtmx_layer_apply(nr, hwde->layer_config_data,
		sunxi_de_get_layer_count(nr));
	sunxi_de_layer_enhance_apply(nr, hwde->layer_config_data,
		sunxi_de_get_layer_count(nr));

	de_rtmx_update_reg_ahb(nr);

	if (hwde->use_rcq) {
		sunxi_de_protect_reg_for_rcq(hwde, false);
		de_top_query_state_with_clear(nr, DE_IRQ_STATE_MASK);
	}

	return 0;
}

/* enhance */
static int sunxi_de_enhance_apply(int nr,
		struct disp_enhance_config *config)
{
	return de_enhance_apply(nr, config);
}

static void sunxi_de_enhance_sync(int nr)
{
	/* struct sunxi_de *hwde = sunxi_de_get_de(nr); */

	de_enhance_update_regs(nr);
	de_enhance_sync(nr);
	de_enhance_tasklet(nr);
}

static void sunxi_de_smbl_sync(int nr)
{
	de_smbl_update_regs(nr);
	de_smbl_tasklet(nr);
}

static irqreturn_t sunxi_de_rcq_finish_irq_handler(int irq, void *arg)
{
	unsigned int irq_state;
	struct sunxi_de *hwde = (struct sunxi_de *)arg;

	irq_state = de_top_query_state_with_clear(hwde->id,
		DISP_AL_IRQ_STATE_RCQ_ACCEPT | DISP_AL_IRQ_STATE_RCQ_FINISH);

	if (irq_state & DISP_AL_IRQ_STATE_RCQ_FINISH) {
		de_rtmx_set_all_rcq_head_dirty(hwde->id, 0);
		atomic_set(&rcq_update_flag, RCQ_UPDATE_FINISHED);
		wake_up(&rcq_update_queue);
	} else if (irq_state & DISP_AL_IRQ_STATE_RCQ_ACCEPT) {
		DRM_INFO("DISP_AL_IRQ_STATE_RCQ_ACCEPT\n");
	}

	return IRQ_HANDLED;
}

static int sunxi_de_enable(int nr, struct disp_manager_data *data)
{
	int ret = 0;
	struct sunxi_de *hwde = sunxi_de_get_de(nr);

	if (hwde->enable) {
		DRM_INFO("[SUNXI-DE]WARN:sunxi has been enable,"
			"do NOT enable it again\n");
		return 0;
	}

	hwde->enable = true;
	memcpy(&hwde->info, &data->config,
		sizeof(hwde->info));

	ret = reset_control_deassert(de_drv->rst_bus_de);
	if (ret) {
		DRM_ERROR("reset_control_deassert for rst_bus_de failed\n\n");
		return -1;
	}

	ret = clk_prepare_enable(de_drv->mclk);
	if (ret < 0) {
		DRM_ERROR("Enable de module clk failed\n\n");
		return -1;
	}

	ret = clk_prepare_enable(de_drv->mclk_bus);
	if (ret < 0) {
		DRM_ERROR("Enable de module bus clk failed\n");
		return -1;
	}

	if (hwde->use_rcq) {
		ret = request_irq(hwde->irq_no, sunxi_de_rcq_finish_irq_handler, 0,
			"sunxi-de-rcq", hwde);
		if (ret) {
			DRM_ERROR("request RCQ irq Failed!\n");
			return -1;
		}
	}

	de_rtmx_start(nr);
#if IS_ENABLED(CONFIG_AW_IOMMU)
		sunxi_enable_device_iommu(DE_MASTOR_ID, true);
#endif
	de_rtmx_mgr_apply(nr, data);
	de_rtmx_update_reg_ahb(nr);

	if (hwde->use_rcq) {
		de_top_enable_irq(nr,
			(DE_IRQ_FLAG_RCQ_FINISH) & DE_IRQ_FLAG_MASK, 1);
		de_top_query_state_with_clear(nr, DE_IRQ_STATE_MASK);
		sunxi_de_protect_reg_for_rcq(hwde, 0);
	}

/* enhance */
	ret = sunxi_de_enhance_apply(nr,
		&hwde->enhance_config);
	if (ret) {
		DRM_ERROR("sunxi_de_enhance_apply failed\n");
		return ret;
	}

/* smbl */
	ret = de_smbl_apply(nr, &hwde->smbl_info);
	if (ret) {
		DRM_ERROR("de_smbl_apply failed\n");
		return ret;
	}

	return 0;
}

/* int sunxi_de_sw_enable(int nr)
{
	struct sunxi_de *hwde = sunxi_de_get_de(nr);

	hwde->is_assigned = true;
	sunxi_de_clk_enable();
	return 0;
} */

static void sunxi_de_disable(int nr, struct disp_manager_data *data)
{
	struct sunxi_de *hwde = sunxi_de_get_de(nr);

	if (!hwde->enable) {
		DRM_INFO("[SUNXI-DE]WARN:sunxi has NOT been enable,"
			"can NOT enable it\n");
		return;
	}

	if (hwde->use_rcq)
		free_irq(hwde->irq_no, hwde);

	hwde->enable = false;
	memcpy(&hwde->info, &data->config,
		sizeof(hwde->info));

	de_top_enable_irq(nr,
			(DE_IRQ_FLAG_RCQ_FINISH)  & DE_IRQ_FLAG_MASK, 0);
	atomic_set(&rcq_update_flag, RCQ_UPDATE_NONE);
	de_rtmx_stop(nr);

	clk_disable_unprepare(de_drv->mclk);
	return;
}


static bool sunxi_de_is_use_irq(int nr)
{
	return false;
}

static unsigned int sunxi_de_get_irq_no(int nr)
{
	return 0;
}

static int sunxi_de_query_irq(int nr)
{
	int tcon_id, irq_query;

	tcon_id = sunxi_de_get_attached_tcon_id(nr);
	if (tcon_id < 0) {
		DRM_ERROR("DE%d has No atteched tcon\n", nr);
		return -1;
	}

	irq_query = sunxi_tcon_query_irq(tcon_id);
	if (!irq_query) {
		DRM_ERROR("irq_query result: No IRQ\n");
		return -1;
	}

	return 0;
}


/*
 * The main task during de vsync is to update de registers and make it effective
 */
static int sunxi_de_event_proc(int nr, bool update)
{
	/* struct sunxi_de *hwde = sunxi_de_get_de(nr); */

	if (!update)
		return 0;

	de_rtmx_update_reg_ahb(nr);

	/* enhance */
	sunxi_de_enhance_sync(nr);

	/* smbl */
	sunxi_de_smbl_sync(nr);

	return 0;
}

void sunxi_updata_crtc_freq(unsigned long rate)
{

}

/* enhance */
static int sunxi_de_set_enhance(int nr,
	int mode, bool enable, int width, int height, int conn_type)
{
	struct sunxi_de *hwde = sunxi_de_get_de(nr);
	struct disp_enhance_config *config = &hwde->enhance_config;

	if (!enable) {
		config->info.enable = 0;
		config->flags |= ENH_ENABLE_DIRTY;
		goto out;
	}

	config->info.mode =
		(config->info.mode & 0xffff) | (mode << 16);
	config->flags |= ENH_MODE_DIRTY;

	if (!width || !height) {
		DRM_ERROR("invalid crtc size, width:%d height:%d\n",
					width, height);
		return -1;
	}

	config->info.size.width = width;
	config->info.size.height = height;
	config->flags |= ENH_SIZE_DIRTY;

	if (conn_type == DISP_OUTPUT_TYPE_HDMI)
		config->info.mode = (config->info.mode & 0xffff0000) | 1;
	else
		config->info.mode = (config->info.mode & 0xffff0000) | 0;

	config->info.enable = 1;
	config->flags |= ENH_ENABLE_DIRTY;

out:
	return sunxi_de_enhance_apply(nr, config);

}

static void sunxi_de_get_enhance(int nr,
	int *mode, bool *enable, int *width, int *height)
{
	struct sunxi_de *hwde = sunxi_de_get_de(nr);
	struct disp_enhance_config *config = &hwde->enhance_config;

	if (mode)
		*mode = config->info.mode;
	if (enable)
		*enable = config->info.enable;
	if (width)
		*width = config->info.size.width;
	if (height)
		*height = config->info.size.height;
}

/* smbl */
static bool sunxi_de_is_support_smbl(int nr)
{
	bool ret;

	ret = de_feat_is_support_smbl(nr) ? true : false;

	return ret;
}

static int sunxi_de_set_smbl(int nr, bool enable,
			struct disp_rect *rect)
{
	struct sunxi_de *hwde = sunxi_de_get_de(nr);
	struct disp_smbl_info *info = &hwde->smbl_info;

	memcpy(&info->window, rect, sizeof(*rect));
	info->flags |= SMBL_DIRTY_WINDOW;

	info->enable = enable;
	info->flags |= SMBL_DIRTY_ENABLE;

	return de_smbl_apply(nr, info);
}

static void sunxi_de_get_smbl(int nr, bool *enable,
			struct disp_rect *rect)
{
	struct sunxi_de *hwde = sunxi_de_get_de(nr);
	struct disp_smbl_info *info = &hwde->smbl_info;

	if (rect)
		memcpy(rect, &info->window, sizeof(*rect));

	if (enable)
		*enable = info->enable;
}

static const struct sunxi_de_funcs de_funcs = {
	.is_support_tcon = sunxi_de_is_support_tcon,
	.get_freq = sunxi_de_get_freq,

	.get_layer_count = sunxi_de_get_layer_count,
	.get_vi_layer_count = sunxi_de_get_vi_layer_count,
	.get_ui_layer_count = sunxi_de_get_ui_layer_count,
	.get_layer_channel_id = sunxi_de_get_layer_channel_id,
	.get_layer_id_within_chanel = sunxi_de_get_layer_id_within_chanel,
	.get_layer_formats = sunxi_de_get_layer_formats,
	.single_layer_apply = sunxi_de_single_layer_apply,
	.multi_layers_apply = sunxi_de_multi_layers_apply,

	.is_enable = sunxi_de_is_enable,
	.enable = sunxi_de_enable,
	.disable = sunxi_de_disable,

	.is_use_irq =  sunxi_de_is_use_irq,
	.get_irq_no = sunxi_de_get_irq_no,
	.query_irq = sunxi_de_query_irq,
	.event_proc = sunxi_de_event_proc,

	/* enhance */
	.set_enhance = sunxi_de_set_enhance,
	.get_enhance = sunxi_de_get_enhance,

	/* smbl */
	.is_support_smbl = sunxi_de_is_support_smbl,
	.set_smbl = sunxi_de_set_smbl,
	.get_smbl = sunxi_de_get_smbl,
};

static ssize_t sunxi_de_reg_dump(char *buf, unsigned int start,
						unsigned int end)
{
	ssize_t n = 0;
	unsigned int *reg, i;

	for (i = start; i <= end; i += 4) {
		/* print address info */
		if (i == start)
			n += sprintf(buf + n, "0x%06x:", i);
		if ((i % 16 == 0) && (i != start)) {
			n += sprintf(buf + n, "%s", "\n");
			n += sprintf(buf + n, "0x%06x:", i);
		}

		/* print reg value */
		reg = (unsigned int *)(de_drv->reg_base + i);
		n += sprintf(buf + n, "0x%08x ", *reg);
	}

	n += sprintf(buf + n, "\n");

	return n;
}

/* print sunxi-de info */
static ssize_t de_info_show(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	ssize_t i, n = 0;
	struct sunxi_de *hwde;

	n += sprintf(buf + n, "%s\n", "sunxi-de info:");
	n += sprintf(buf + n, "virt_reg_base: 0x%lx\n", de_drv->reg_base);
	n += sprintf(buf + n, "de count: %u\n", de_drv->de_cnt);
	n += sprintf(buf + n, "clk_name:%s clk_rate: %lu  enable count:%d\n\n",
					__clk_get_name(de_drv->mclk),
					clk_get_rate(de_drv->mclk),
					__clk_get_enable_count(de_drv->mclk));

	for (i = 0; i < de_drv->de_cnt; i++) {
		hwde = &de_drv->hwde[i];
		n += sprintf(buf + n, "id: %d\n", hwde->id);
		n += sprintf(buf + n, "vi channel count: %u\n",
					hwde->vichannel_cnt);
		n += sprintf(buf + n, "ui channel count: %u\n",
					hwde->uichannel_cnt);
		n += sprintf(buf + n, "vi layers per channel: %u\n",
					hwde->layers_per_vichannel);
		n += sprintf(buf + n, "ui layers per channel: %u\n",
					hwde->layers_per_uichannel);
		if (!hwde->irq_used) {
			n += sprintf(buf + n, "%s\n", "de irq is NOT used");
		} else {
			n += sprintf(buf + n, "%s\n", "de irq is used");
			n += sprintf(buf + n, "irq no: %u\n", hwde->irq_no);
			n += sprintf(buf + n, "irq enable: %d\n",
							hwde->irq_enable);
		}

		if (hwde->enable)
			n += sprintf(buf + n, "%s\n", "Has been enable");
		else
			n += sprintf(buf + n, "%s\n", "Has NOT been enable");
		n += sprintf(buf + n, "%s", "\n");
	}

	return n;
}

static ssize_t de_info_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	return count;
}

static DEVICE_ATTR(info, 0660, de_info_show, de_info_store);

static ssize_t de_top_reg_show(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	ssize_t n = 0;

	if (!__clk_is_enabled(de_drv->mclk)) {
		n += sprintf(buf + n, "%s\n",
			"ERROR:de clk is NOT enabled, can NOT dump reg");
		return n;
	}

	n += sprintf(buf + n, "TOP DE reg:\n");
	n += sunxi_de_reg_dump(n + buf, 0, 0x1c);
	return n;
}

static ssize_t de_top_reg_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	return count;
}

static DEVICE_ATTR(top_reg, 0660, de_top_reg_show, de_top_reg_store);

static ssize_t de_rtmx_reg_show(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	ssize_t n = 0;
	int i, j;
	bool vi_en = 0, ui_en = 0;
	unsigned int start_addr, end_addr;
	struct sunxi_de *hwde = &de_drv->hwde[0];

	n += sprintf(buf + n, "RT-Mix0-glb reg:\n");
	n += sunxi_de_reg_dump(n + buf, 0x100000, 0x10000c);

	n += sprintf(buf + n, "RT-Mix0-bld reg:\n");
	start_addr = 0x101000;
	end_addr = start_addr + 0xfc;
	n += sunxi_de_reg_dump(n + buf, start_addr, end_addr);

	n += sprintf(buf + n, "vichannel status:\n");
	for (i = 0; i < hwde->vichannel_cnt; i++) {
		vi_en = 0;
		for (j = 0; j < hwde->layers_per_vichannel; j++) {
			if (!hwde->layer_enabled[i][j]) {
				n += sprintf(buf + n, "CH%d-LAYER%d NOT enabled\n", i, j);
				continue;
			}
			n += sprintf(buf + n, "CH%d-LAYER%d IS enabled\n", i, j);

			n += sprintf(buf + n, "CH%d-LAYER%d reg:\n", i, j);
			n += sprintf(buf + n, "VI General control reg:\n");
			start_addr = 0x102000 + (i * 0x1000) + j * 0x30;
			end_addr = start_addr + 0x2c;
			n += sunxi_de_reg_dump(n + buf, start_addr, end_addr);
			n += sprintf(buf + n, "%s", "\n");

			n += sprintf(buf + n, "VI fill color reg:\n");
			start_addr = 0x102000 + (i * 0x1000) + j * 0x4;
			end_addr = start_addr + 0;
			n += sunxi_de_reg_dump(n + buf, start_addr, end_addr);
			n += sprintf(buf + n, "%s", "\n");
			vi_en = 1;
		}

		/* No layer in this channel is enabled */
		if (!vi_en)
			continue;

		n += sprintf(buf + n, "VI TOP reg:\n");
		start_addr = 0x102000 + (i * 0x1000) + 0xd0;
		end_addr = 0x102000 + (i * 0x1000) + 0xfc;
		n += sunxi_de_reg_dump(n + buf, start_addr, end_addr);
		n += sprintf(buf + n, "%s", "\n\n");
	}

	n += sprintf(buf + n, "uichannel status:\n");
	for (i = 0; i < hwde->uichannel_cnt; i++) {
		ui_en = 0;
		for (j = 0; j < hwde->layers_per_uichannel; j++) {
			if (!hwde->layer_enabled[i + hwde->vichannel_cnt][j]) {
				n += sprintf(buf + n, "CH%d-LAYER%d NOT enabled\n",
						i + hwde->vichannel_cnt, j);
				continue;
			}
			n += sprintf(buf + n, "CH%d-LAYER%d reg:\n",
					i + hwde->vichannel_cnt, j);
			n += sprintf(buf + n, "UI General control reg:\n");
			start_addr = 0x102000 + (0x1000 * hwde->vichannel_cnt)
						+ (i * 0x1000) + j * 0x20;
			end_addr = start_addr + 0x18;
			n += sunxi_de_reg_dump(n + buf, start_addr, end_addr);
			n += sprintf(buf + n, "%s", "\n");
			ui_en = 1;
		}

		/* No layer in this channel is enabled */
		if (!ui_en)
			continue;

		n += sprintf(buf + n, "CH%d UI TOP reg:\n", i + hwde->vichannel_cnt);
		start_addr = 0x102000 + (0x1000 * hwde->vichannel_cnt)
						+ (i * 0x1000) + 0x80;
		end_addr = 0x102000 + (0x1000 * hwde->vichannel_cnt)
						+ (i * 0x1000) + 0x88;
		n += sunxi_de_reg_dump(n + buf, start_addr, end_addr);
		n += sprintf(buf + n, "%s", "\n\n");
	}

	n += sprintf(buf + n, "%s", "\n");
	return n;
}

static ssize_t de_rtmx_reg_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	return count;
}

static DEVICE_ATTR(rtmx_reg, 0660, de_rtmx_reg_show, de_rtmx_reg_store);


static ssize_t de_vsu_reg_show(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	ssize_t n = 0;
	int i, j, vi_chn_cnt;
	unsigned int start_addr, end_addr;
	struct sunxi_de *hwde = &de_drv->hwde[0];

	if (!__clk_is_enabled(de_drv->mclk)) {
		n += sprintf(buf + n, "%s\n",
			"ERROR:de clk is NOT enabled, can NOT dump reg");
		return n;
	}

	vi_chn_cnt = hwde->vichannel_cnt;
	for (i = 0; i < vi_chn_cnt; i++) {
		for (j = 0; j < hwde->layers_per_vichannel; j++) {
			if (hwde->layer_enabled[i][j])
				break;
		}

		if (j >= hwde->layers_per_vichannel) {
			n += sprintf(buf + n, "Video CH%d is NOT enabled\n", i);
			continue;
		}

		n += sprintf(buf + n, "CH%d VSU reg:\n", i);
		start_addr = 0x00100000 + ((i + 1) * 0x20000);
		end_addr = start_addr + 0xdc;
		n += sunxi_de_reg_dump(n + buf, start_addr, end_addr);
	}

	return n;
}

static ssize_t de_vsu_reg_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	return count;
}

static DEVICE_ATTR(vsu_reg, 0660, de_vsu_reg_show, de_vsu_reg_store);

static ssize_t de_gsu_reg_show(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	ssize_t n = 0;
	int i, j, vi_chn_cnt, ui_chn_cnt;
	unsigned int start_addr, end_addr;
	struct sunxi_de *hwde = &de_drv->hwde[0];

	if (!__clk_is_enabled(de_drv->mclk)) {
		n += sprintf(buf + n, "%s\n",
			"ERROR:de clk is NOT enabled, can NOT dump reg");
		return n;
	}

	vi_chn_cnt = hwde->vichannel_cnt;
	ui_chn_cnt = hwde->uichannel_cnt;
	for (i = 0; i < ui_chn_cnt; i++) {
		for (j = 0; j < hwde->layers_per_uichannel; j++) {
			if (hwde->layer_enabled[i + vi_chn_cnt][j])
				break;
		}

		if (j >= hwde->layers_per_uichannel) {
			n += sprintf(buf + n, "UI CHANNEL, CH%d is NOT enabled\n",
					i + vi_chn_cnt);
			continue;
		}

		n += sprintf(buf + n, "CH%d GSU reg:\n", i + vi_chn_cnt);
		start_addr = 0x00100000 + (vi_chn_cnt * 0x20000) + (i * 0x10000);
		end_addr = start_addr + 0x9c;
		n += sunxi_de_reg_dump(n + buf, start_addr, end_addr);
	}

	return n;
}

static ssize_t de_gsu_reg_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	return count;
}

static DEVICE_ATTR(gsu_reg, 0660, de_gsu_reg_show, de_gsu_reg_store);


/* enhance is belongs to POST-PROCESS1,
 * Only use in video chanel
 */
static ssize_t de_enhance_reg_show(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	ssize_t n = 0;
	unsigned int start_addr, end_addr;

	if (!__clk_is_enabled(de_drv->mclk)) {
		n += sprintf(buf + n, "%s\n",
			"ERROR:de clk is NOT enabled, can NOT dump reg");
		return n;
	}

	n += sprintf(buf + n, "FCE reg:\n");
	start_addr = 0x00100000 + 0xa0000;
	end_addr = start_addr + 0x40;
	n += sunxi_de_reg_dump(n + buf, start_addr, end_addr);

	n += sprintf(buf + n, "LTI reg:\n");
	start_addr = 0x00100000 + 0xa4000;
	end_addr = start_addr + 0xcc;
	n += sunxi_de_reg_dump(n + buf, start_addr, end_addr);

	n += sprintf(buf + n, "PEAK reg:\n");
	start_addr = 0x00100000 + 0xa6000;
	end_addr = start_addr + 0xcc;
	n += sunxi_de_reg_dump(n + buf, start_addr, end_addr);

	n += sprintf(buf + n, "ASE reg:\n");
	start_addr = 0x00100000 + 0xa8000;
	end_addr = start_addr + 0x10;
	n += sunxi_de_reg_dump(n + buf, start_addr, end_addr);

	n += sprintf(buf + n, "FCC reg:\n");
	start_addr = 0x00100000 + 0xaa000;
	end_addr = start_addr + 0x90;
	n += sunxi_de_reg_dump(n + buf, start_addr, end_addr);

	return n;
}

static ssize_t de_enhance_reg_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	return count;
}

static DEVICE_ATTR(enhance_reg, 0660, de_enhance_reg_show, de_enhance_reg_store);

static ssize_t sunxi_disp_hal_debug_show(struct device *device,
			   struct device_attribute *attr,
			   char *buf)
{
	ssize_t n = 0;

	return n;
}

static ssize_t sunxi_disp_hal_debug_store(struct device *device,
			   struct device_attribute *attr,
			   const char *buf, size_t count)
{
	unsigned int level;

	if (kstrtoul(buf, 0, (unsigned long *)&level) < 0) {
		DRM_ERROR("ERROR input\n");
		return 0;
	}

	return count;
}

static DEVICE_ATTR(disp_hal_debug, 0660,
	sunxi_disp_hal_debug_show, sunxi_disp_hal_debug_store);

static ssize_t sunxi_disp_debug_show(struct device *device,
			   struct device_attribute *attr,
			   char *buf)
{
	ssize_t n = 0;

	n += sprintf(buf + n, "disp_debug=%d\n",
				bsp_disp_get_print_level());

	return n;
}

static ssize_t sunxi_disp_debug_store(struct device *device,
			   struct device_attribute *attr,
			   const char *buf, size_t count)
{
	unsigned int debug;

	if (kstrtoul(buf, 0, (unsigned long *)&debug) < 0) {
		DRM_ERROR("ERROR input\n");
		return 0;
	}

	bsp_disp_set_print_level(debug);

	return count;
}

static DEVICE_ATTR(disp_debug, 0660, sunxi_disp_debug_show, sunxi_disp_debug_store);

static struct attribute *de_attributes[] = {
	&dev_attr_info.attr,
	&dev_attr_top_reg.attr,
	&dev_attr_rtmx_reg.attr,
	&dev_attr_vsu_reg.attr,
	&dev_attr_gsu_reg.attr,
	&dev_attr_enhance_reg.attr,
	&dev_attr_disp_debug.attr,
	&dev_attr_disp_hal_debug.attr,
	NULL
};

static struct attribute_group de_attribute_group = {
	.name = "attr",
	.attrs = de_attributes,
};

static int sunxi_de_v33x_al_init(struct disp_bsp_init_para *para)
{
	if (de_top_mem_pool_alloc())
		return -1;

	de_enhance_init(para);
	de_smbl_init(para->reg_base[DISP_MOD_DE]);
	de_rtmx_init(para);
	de_wb_init(para);

	return 0;
}

static int sunxi_de_v33x_al_exit(void)
{
	return de_rtmx_exit();
}

static int sunxi_de_init_al(void)
{
	struct disp_bsp_init_para *para;

	sunxi_disp_bsp_init_para_init();
	para = sunxi_disp_get_bsp_init_para();
	if (!para) {
		DRM_ERROR("[sunxi-de] sunxi_disp_get_bsp_init_para failed\n");
		return -1;
	}

	para->reg_base[DISP_MOD_DE] = de_drv->reg_base;
	para->mclk[DISP_MOD_DE] = de_drv->mclk;
	para->irq_no[DISP_MOD_DE] = de_drv->irq_no;

	return sunxi_de_v33x_al_init(para);
}

/* parse de dts info: reg_base/clk */
static int sunxi_de_parse_dts(struct platform_device *pdev)
{
	struct device_node *node;

	node = pdev->dev.of_node;
	if (!node) {
		DRM_ERROR("get sunxi-de node err.\n ");
		return -EINVAL;
	}

/* parse de reg_base */
	de_drv->reg_base = (uintptr_t __force)of_iomap(node, 0);
	if (!de_drv->reg_base) {
		DRM_ERROR("unable to map de registers\n");
		return -EINVAL;
	}
	DRM_DEBUG_DRIVER("[SUNXI-DE]get de reg_base:0x%lx\n", de_drv->reg_base);

/* get clk of de */
	de_drv->mclk = devm_clk_get(&pdev->dev, "clk_de");
	if (IS_ERR(de_drv->mclk)) {
		DRM_ERROR("fail to get clk for de\n");
		return -EINVAL;
	}

	de_drv->mclk_bus = devm_clk_get(&pdev->dev, "clk_bus_de");
	if (IS_ERR(de_drv->mclk)) {
		DRM_ERROR("fail to get bus clk for de\n");
		return -EINVAL;
	}

	de_drv->rst_bus_de = devm_reset_control_get_shared(&pdev->dev, "rst_bus_de");
	if (IS_ERR(de_drv->rst_bus_de)) {
		DRM_ERROR("fail to get reset clk for rst_bus_de\n");
		return -EINVAL;
	}

	de_drv->irq_no = irq_of_parse_and_map(node, 0);
	if (!de_drv->irq_no) {
		DRM_ERROR("irq_of_parse_and_map de irq fail\n");
		return -EINVAL;
	}

	if (of_property_read_u32(node, "chn_cfg_mode",
		&de_drv->chn_cfg_mode) < 0) {
		DRM_ERROR("failed to read chn_cfg_mode\n");
		return -EINVAL;
	}

	return 0;
}

static int sunxi_de_probe(struct platform_device *pdev)
{
	int ret, i, lay_num;
	struct de_feat_init de_feat;

	DRM_INFO("[SUNXI-DE] sunxi_de_probe start\n");

	de_drv->pdev = pdev;

/* Parse DTS INFO */
	ret = sunxi_de_parse_dts(pdev);
	if (ret < 0) {
		DRM_ERROR("Parse de dts failed!\n");
		goto de_err;
	}

/* init de_feat */
	de_feat.chn_cfg_mode = de_drv->chn_cfg_mode;
	de_feat_init_config(&de_feat);

	de_drv->de_cnt = de_feat_get_num_screens();
	if ((de_drv->de_cnt <= 0) || (de_drv->de_cnt > DE_NUM_MAX)) {
		DRM_ERROR("get wrong de count");
		goto de_err;
	}

/* init struct sunxi_de */
	for (i = 0; i < de_drv->de_cnt; i++) {
		struct sunxi_de *hwde = &de_drv->hwde[i];

		hwde->id = i;

		hwde->vichannel_cnt = de_feat_get_num_vi_chns(i);
		hwde->uichannel_cnt = de_feat_get_num_ui_chns(i);
		hwde->layers_per_vichannel = 4;
		hwde->layers_per_uichannel = 4;
	}

	for (i = 0; i < de_drv->de_cnt; i++) {
		struct sunxi_de *hwde = &de_drv->hwde[i];

		lay_num = sunxi_de_get_layer_count(i);
		hwde->layer_config_data =
			kzalloc(lay_num * sizeof(struct disp_layer_config_data),
						GFP_KERNEL);
		if (!hwde->layer_config_data) {
			DRM_ERROR("KZALLOC for disp_layer_config_data failed\n");
			return -1;
		}

		/* in DE_V33X, IRQ is rcq irq which is a internal irq */
		hwde->irq_no = de_drv->irq_no;
		hwde->irq_used = 0;
		hwde->irq_enable = 0;

		hwde->use_rcq = de_feat_is_using_rcq(i);

		hwde->funcs = &de_funcs;
	}


	if (sunxi_de_init_al() < 0) {
		DRM_ERROR("sunxi_de_init_al failed!\n");
		goto de_err;
	}

	init_waitqueue_head(&rcq_update_queue);
#if defined(CONFIG_AW_IOMMU)
	/* sunxi_enable_device_iommu(DE_MASTOR_ID, true); */
#endif
	DRM_INFO("[SUNXI-DE] sunxi_de_probe end\n");
	return 0;

de_err:
	DRM_ERROR("sunxi_de_probe FAILED\n");
	return -EINVAL;
}

static int sunxi_de_remove(struct platform_device *pdev)
{
	return sunxi_de_v33x_al_exit();
}

static const struct of_device_id sunxi_de_match[] = {
	{ .compatible = "allwinner,sunxi-de", },
	{},
};

struct platform_driver sunxi_de_platform_driver = {
	.probe = sunxi_de_probe,
	.remove = sunxi_de_remove,
	.driver = {
		   .name = "de",
		   .owner = THIS_MODULE,
		   .of_match_table = sunxi_de_match,
	},
};

int sunxi_de_module_init(void)
{
	int ret = 0, err;
	struct drv_model_info  *drv_model;

	DRM_INFO("[SUNXI-DE]sunxi_de_module_init\n");

	de_drv = kzalloc(sizeof(*de_drv), GFP_KERNEL);
	if (!de_drv) {
		DRM_ERROR("can NOT allocate memory for de_drv\n");
		goto de_err;
	}

	drv_model = &de_drv->drv_model;
	if (alloc_chrdev_region(&drv_model->devid, 0, 1, "de") < 0) {
		DRM_ERROR("alloc_chrdev_region failed\n");
		goto de_err;
	}

	drv_model->cdev = cdev_alloc();
	if (!drv_model->cdev) {
		DRM_ERROR("cdev_alloc failed\n");
		goto de_err;
	}

	cdev_init(drv_model->cdev, NULL);
	drv_model->cdev->owner = THIS_MODULE;
	err = cdev_add(drv_model->cdev, drv_model->devid, 1);
	if (err) {
		DRM_ERROR("cdev_add major number:%d failed\n",
						MAJOR(drv_model->devid));
		goto de_err;
	}

	drv_model->sysclass = class_create(THIS_MODULE, "de");
	if (IS_ERR(drv_model->sysclass)) {
		DRM_ERROR("create class error\n");
		goto de_err;
	}

	drv_model->dev = device_create(drv_model->sysclass, NULL,
					drv_model->devid, NULL, "de");
	if (!drv_model->dev) {
		DRM_ERROR("device_create failed\n");
		goto de_err;
	}

	ret = platform_driver_register(&sunxi_de_platform_driver);
	if (ret) {
		DRM_ERROR("platform_driver_register failed\n");
		goto de_err;
	}

	ret = sysfs_create_group(&drv_model->dev->kobj, &de_attribute_group);
	if (ret < 0) {
		DRM_ERROR("sysfs_create_file fail!\n");
		goto de_err;
	}

	DRM_INFO("[SUNXI-DE]sunxi_de_module_init end\n\n");
	return 0;
de_err:
	kfree(de_drv);
	DRM_ERROR("sunxi_de_module_init FAILED\n");
	return -EINVAL;
}

void sunxi_de_module_exit(void)
{
	struct drv_model_info           *drv_model;

	DRM_INFO("[SUNXI-DE]sunxi_de_module_exit\n");

	drv_model = &de_drv->drv_model;
	platform_driver_unregister(&sunxi_de_platform_driver);
	device_destroy(drv_model->sysclass, drv_model->devid);
	class_destroy(drv_model->sysclass);

	cdev_del(drv_model->cdev);
	kfree(de_drv);
}

