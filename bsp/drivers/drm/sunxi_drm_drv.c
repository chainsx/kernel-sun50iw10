/* sunxi_drm_drv.c
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

#include <drm/drm_crtc_helper.h>
#include <drm/drm_drv.h>
#include <drm/drm_probe_helper.h>

#include "sunxi_drm_bootlogo.h"
#include "sunxi_drm_drv.h"
#include "sunxi_drm_crtc.h"
#include "sunxi_drm_encoder.h"
#include "sunxi_drm_connector.h"
#include "sunxi_drm_fb.h"
#include "sunxi_drm_gem.h"

#ifdef CONFIG_DRM_FBDEV_EMULATION
#include "sunxi_drm_fbdev.h"
#endif

#include "sunxi_drm_gem.h"
#include "sunxi_drm_sysfs.h"
#include "sunxi_drm_iommu.h"

#include "drm_internal.h"

#define DRIVER_NAME	"sunxi-drm"
#define DRIVER_DESC	"allwinnertech SoC DRM"
#define DRIVER_DATE	"20220517"
#define DRIVER_MAJOR	2
#define DRIVER_MINOR	0

#define VBLANK_OFF_DELAY	50000

static struct sunxi_drm_private sunxi_drv_prv;

static int sunxi_drm_platform_init(struct drm_driver *ddrv, struct device *dev)
{
	struct drm_device *ddev;
	struct platform_device *pdev = to_platform_device(dev);
	struct device_node *node = dev->of_node;
	int ret;

	ddev = drm_dev_alloc(ddrv, dev);
	if (IS_ERR(ddev))
		return PTR_ERR(ddev);

	ret = drm_dev_register(ddev, 0);
	if (ret)
		goto init_failed;

	DRM_INFO("Initialized %s %d.%d.%d %s on minor %d\n",
		 ddrv->name, ddrv->major, ddrv->minor, ddrv->patchlevel,
		 ddrv->date, ddev->primary->index);
	return 0;

init_failed:
		/* @TODO */
	return ret;
}

struct drm_device *sunxi_drm_get_drm_device(void)
{
	return sunxi_drv_prv.drm_dev;
}

#ifdef CONFIG_VIDEO_SUNXI_CAR_REVERSE
static int _force_plane_en;
static unsigned int active_plane_mask;

int sunxi_drm_get_force_plane_en(void)
{
	return _force_plane_en;
}

int sunxi_drm_get_num_crtc(void)
{
	return sunxi_drm_get_crtc_count();
}
EXPORT_SYMBOL(sunxi_drm_get_num_crtc);

static int sunxi_drm_crtc_find_attached_device(struct drm_crtc *crtc,
		struct drm_encoder **enc, struct drm_connector **conn)
{
	bool find = false;
	struct drm_encoder *encoder;
	struct drm_connector *connector;
	struct drm_device *dev;

	dev = crtc->dev;

	/* find attaching encoder for crtc */
	find = false;
	drm_for_each_encoder(encoder, dev) {
		if (encoder->crtc == crtc) {
			find = true;
			break;
		}
	}
	if (!find) {
		DRM_ERROR("crtc:%d has NOT attaching encoder!\n", crtc->index);
		return -1;
	}
	*enc = encoder;

	/* find attaching connector for encoder */
	find = false;
	drm_for_each_connector(connector, dev) {
		if (connector->encoder == encoder) {
			find = true;
			break;
		}
	}
	if (!find) {
		DRM_ERROR("encoder:%d has NOT attaching connector!\n",
			encoder->index);
		return -1;
	}
	*conn = connector;

	return 0;
}

static void disp_layer_config_convert_to_inner(
		struct disp_layer_config_inner *layer_inner,
		struct disp_layer_config *config)
{
	layer_inner->enable = config->enable;
	layer_inner->channel = config->channel;
	layer_inner->layer_id = config->layer_id;
	layer_inner->info.mode = config->info.mode;
	layer_inner->info.zorder = config->info.zorder;
	layer_inner->info.alpha_mode = config->info.alpha_mode;
	layer_inner->info.alpha_value = config->info.alpha_value;
	memcpy(&layer_inner->info.screen_win,
		&config->info.screen_win, sizeof(config->info.screen_win));
	memcpy(&layer_inner->info.fb.addr, &config->info.fb.addr,
		3 * sizeof(unsigned long long));
	memcpy(&layer_inner->info.fb.size, &config->info.fb.size,
		3 * sizeof(struct disp_rectsz));
	memcpy(&layer_inner->info.fb.align, &config->info.fb.align,
		3 * sizeof(unsigned int));
	layer_inner->info.fb.format = config->info.fb.format;
	layer_inner->info.fb.color_space = config->info.fb.color_space;
	layer_inner->info.fb.pre_multiply = config->info.fb.pre_multiply;
	memcpy(&layer_inner->info.fb.crop, &config->info.fb.crop,
		sizeof(config->info.fb.crop));
	layer_inner->info.fb.flags = config->info.fb.flags;
	layer_inner->info.fb.scan = config->info.fb.scan;
}

static void sunxi_drm_disable_all_active_plane(struct drm_crtc *crtc)
{
	struct drm_plane *plane;
	struct sunxi_drm_crtc *scrtc = to_sunxi_crtc(crtc);

	drm_for_each_plane(plane, crtc->dev) {
		struct disp_layer_config_data layer_config;

		if (plane->crtc != crtc)
			continue;

		if (!plane->state)
			continue;

		memset(&layer_config, 0,
			sizeof(layer_config));
		layer_config.flag = LAYER_ALL_DIRTY;
		layer_config.config.enable = false;

		layer_config.config.channel =
			scrtc->hw_funcs->get_layer_channel_id(
				scrtc->crtc_id, plane->index);
		layer_config.config.layer_id =
			scrtc->hw_funcs->get_layer_id_within_chanel(
				scrtc->crtc_id, plane->index);

		if (scrtc->hw_funcs->single_layer_apply(scrtc->crtc_id,
					&layer_config) < 0) {
			DRM_ERROR("sunxi_de single layer_apply failed\n");
		} else {
			DRM_DEBUG_DRIVER("%s success\n", __func__);
		}
	}
}

int sunxi_drm_force_set_plane(int crtc_id, struct disp_layer_config *config,
			unsigned int plane_num)
{
	int i = 0, irq_no;
	bool enable;

	unsigned int hdisplay, vdisplay, vrefresh;
	struct disp_video_timings *p_info = NULL;
	unsigned int tv_mode = 0;

	struct disp_manager_data data;
	struct disp_manager_info *info = &data.config;
	struct disp_layer_config_data layer_data[16];
	struct disp_layer_config_inner *layer_inner;

	struct drm_device *dev;
	struct drm_encoder *encoder;
	struct sunxi_drm_encoder *sencoder;
	struct drm_connector *conn;
	struct sunxi_drm_connector *sconn;
	struct sunxi_drm_crtc *scrtc = sunxi_drm_crtc_get_crtc(crtc_id);
	struct drm_crtc_state *crtc_state = NULL;

	dev = scrtc->crtc.dev;
	memset(&data, 0, sizeof(data));
	memset(layer_data, 0,
		16 * sizeof(layer_data[0]));

	_force_plane_en = 1;

	crtc_state = scrtc->crtc.state;
	if (!crtc_state) {
		DRM_ERROR("crtc:%d has NOT set!\n", crtc_id);
		return;
	}

	mutex_lock(&dev->mode_config.mutex);

	if (sunxi_drm_crtc_find_attached_device(&scrtc->crtc,
		&encoder, &conn) < 0) {
		DRM_INFO("[warning] crtc:%d has NOT been set\n",
			scrtc->crtc_id);
		return -1;
	}
	sencoder = to_sunxi_encoder(encoder);
	sconn = to_sunxi_connector(conn);
	/* attaching connector type for tcon */
	sencoder->hw_funcs =
		sunxi_tcon_attach_connector_type(sencoder->encoder_id,
						sconn->type);
	mutex_unlock(&dev->mode_config.mutex);

	enable = scrtc->hw_funcs->is_enable(crtc_id);
	if (!enable) {
	/* irq register */
		sencoder->use_irq =
			sencoder->hw_funcs->is_use_irq(sencoder->encoder_id);
		sencoder->irq_no =
			sencoder->hw_funcs->get_irq_no(sencoder->encoder_id);

		if (sconn->use_irq)
			irq_no = sconn->irq_no;
		else if (sencoder->use_irq)
			irq_no = sencoder->irq_no;
		else
			DRM_INFO("WARN: NO irq for tcon%d and lcd%d\n",
				sencoder->encoder_id, sconn->con_id);
		if (scrtc->irq_register(scrtc, irq_no) < 0) {
			DRM_ERROR("sunxi_drm_crtc_irq_register failed\n");
			return -1;
		}

	/* setting params for sunxi-crtc and enable it */
		data.flag = MANAGER_ALL_DIRTY;
		info->size.x = 0;
		info->size.y = 0;

		info->hwdev_index = encoder->index;

		sconn->get_init_resolution(sconn,
			&hdisplay, &vdisplay, &vrefresh);
		info->size.width = hdisplay;
		info->size.height = vdisplay;

		info->color_space = DISP_BT601_F;

		if (sconn->get_work_mode) {
			struct sunxi_connector_work_mode conn_work_mode;

			sconn->get_work_mode(sconn, &conn_work_mode);

			if (conn_work_mode.color_fmt == COLOR_FMT_RGB444)
				info->cs = DISP_CSC_TYPE_RGB;
			else if (conn_work_mode.color_fmt == COLOR_FMT_YUV444)
				info->cs = DISP_CSC_TYPE_YUV444;
			else if (conn_work_mode.color_fmt == COLOR_FMT_YUV422)
				info->cs = DISP_CSC_TYPE_YUV422;
			else if (conn_work_mode.color_fmt == COLOR_FMT_YUV420)
				info->cs = DISP_CSC_TYPE_YUV420;
			else {
				info->cs = DISP_CSC_TYPE_RGB;
				DRM_ERROR("sunxi connector:%d working mode hasn't been set\n",
					sconn->con_id);
			}
		} else {
			info->cs = DISP_CSC_TYPE_RGB;
		}


		if (info->cs == DISP_CSC_TYPE_RGB)
			info->color_range = DISP_COLOR_RANGE_0_255;
		else
			info->color_range = DISP_COLOR_RANGE_16_235;

		info->conn_type = sconn->type;

		info->enable = true;
		info->disp_device = crtc_id;

		info->blank = false;
		info->de_freq = scrtc->hw_funcs->get_freq(crtc_id);
		info->device_fps = vrefresh;
		info->eotf = DISP_EOTF_GAMMA22;
		info->data_bits = DISP_DATA_8BITS;

		scrtc->hw_funcs->enable(crtc_id, &data);

	/* set encoder and connector */
		if (kstrtou32(crtc_state->mode.name, 10, &tv_mode)) {
			DRM_ERROR("Get tv mode fail!\n");
			return;
		}

		if (sconn->get_video_timing)
			sconn->get_video_timing(p_info, tv_mode);
		sencoder->hw_funcs->set(scrtc->crtc_id,
					sencoder->encoder_id,
					sconn->type_id, p_info, tv_mode);
		sconn->enable(sconn, tv_mode);
	} else {
		sunxi_drm_disable_all_active_plane(&scrtc->crtc);
	}

/* set plane */
	for (i = 0; i < plane_num; i++) {
		layer_data[i].flag = LAYER_ALL_DIRTY;

		layer_inner = &layer_data[i].config;
		disp_layer_config_convert_to_inner(layer_inner,
						&config[i]);
		active_plane_mask |=
			1 << (layer_inner->channel * 4 + layer_inner->layer_id);
	}

	scrtc->hw_funcs->multi_layers_apply(scrtc->crtc_id,
		layer_data, plane_num);

	atomic_set(&scrtc->update, 1);

	return 0;
}
EXPORT_SYMBOL(sunxi_drm_force_set_plane);

/*
  * exit force_set_plane and restore drm setings set by user space app
  */
void sunxi_drm_force_set_plane_exit(int crtc_id)
{
	struct sunxi_drm_crtc *scrtc = sunxi_drm_crtc_get_crtc(crtc_id);
	const struct drm_crtc_helper_funcs *crtc_funcs = scrtc->crtc.helper_private;
	struct drm_crtc_state *crtc_state = NULL;
	struct drm_plane *plane;
	struct drm_plane_state *plane_state = NULL;

	if (!_force_plane_en)
		return;
	_force_plane_en = 0;

	crtc_state = scrtc->crtc.state;
	if (!crtc_state) {
		DRM_ERROR("crtc:%d has NOT set!\n", crtc_id);
		return;
	}

	drm_for_each_plane(plane, scrtc->crtc.dev) {
		struct disp_layer_config_data layer_config;

		if (!(active_plane_mask & (1 << plane->index)))
			continue;

		memset(&layer_config, 0,
			sizeof(layer_config));

		layer_config.config.channel =
			scrtc->hw_funcs->get_layer_channel_id(
				crtc_id, plane->index);
		layer_config.config.layer_id =
			scrtc->hw_funcs->get_layer_id_within_chanel(
				crtc_id, plane->index);

		layer_config.flag = LAYER_ALL_DIRTY;
		layer_config.config.enable = false;

		if (scrtc->hw_funcs->single_layer_apply(scrtc->crtc_id,
					&layer_config) < 0) {
			DRM_ERROR("sunxi_de single layer_apply failed\n");
		} else {
			DRM_DEBUG_DRIVER("%s success\n", __func__);
		}
	}

	drm_for_each_plane(plane, scrtc->crtc.dev) {
		const struct drm_plane_helper_funcs *plane_funcs;

		if (crtc_state->plane_mask & (1 << plane->index)) {
			plane_state = plane->state;
			if (!plane_state)
				continue;

			DRM_INFO("SET plane:%d\n", plane->index);
			plane_funcs = plane->helper_private;
			plane_funcs->atomic_update(plane, NULL);
		}
	}

	atomic_set(&scrtc->update, 1);

	crtc_funcs->enable(&scrtc->crtc);
}
EXPORT_SYMBOL(sunxi_drm_force_set_plane_exit);

#endif

int sunxi_drm_enable_vblank(struct drm_device *dev, unsigned int pipe)
{
	return 0;
}

void sunxi_drm_disable_vblank(struct drm_device *dev, unsigned int pipe)
{
	return;
}

u32 sunxi_drm_vblank_counter(struct drm_device *dev, unsigned int pipe)
{
	return 0;
}

/*
 * bootlogo processing for crtc/connector state:
 * 1.create a state for connector and initial its members with booting information;
 * 2.create a state for crtc and initial its members with booting information;
 */
static int sunxi_drm_smooth_set_state(struct drm_device *dev)
{
	struct drm_display_mode *mode;
	struct drm_crtc *crtc;
	struct sunxi_drm_crtc *scrtc;
	struct sunxi_drm_connector *sconn =
		sunxi_drm_connector_get_connector(0);
	struct drm_connector *conn = &sconn->connector;

	if (!conn->state) {
		conn->state
			= kzalloc(sizeof(*conn->state),
							GFP_KERNEL);
		if (!conn->state) {
			DRM_ERROR("allocate mem for "
				"drm_connector_state failed\n");
			return -1;
		}
	}

	/* sunxi_drm_fbdev_initial_config()
	has been do a default attachment for crtc/encoder/connector */
	if (!conn->encoder) {
		DRM_ERROR("connector has NOT attched to a encoder\n");
		return -1;
	}
	if (!conn->encoder->crtc) {
		DRM_ERROR("encoder has NOT attached to a crtc\n");
		return -1;
	}

	conn->state->best_encoder = conn->encoder;
	conn->state->crtc = conn->encoder->crtc;
	conn->state->connector = conn;

	crtc = conn->encoder->crtc;
	scrtc = to_sunxi_crtc(crtc);

	if (!crtc->state) {
		crtc->state =
			kzalloc(sizeof(*crtc->state), GFP_KERNEL);
		if (!crtc->state) {
			DRM_ERROR("allocate mem for drm_crtc_state "
						"failed\n");
			return -1;
		}
	}

	crtc->state->crtc = crtc;
	crtc->state->enable = 1;
	crtc->state->active = 1;
	crtc->state->plane_mask = 0x10000000;
	crtc->state->connector_mask = 0x1;
	crtc->state->encoder_mask = 1 << conn->encoder->index;

	list_for_each_entry(mode, &conn->modes, head) {
		memcpy(&crtc->state->mode, mode,
			sizeof(*mode));
		drm_mode_copy(&crtc->state->adjusted_mode, mode);
	}

	return 0;
}


bool sunxi_drm_is_need_smooth_boot(void)
{
	/* fb_base got from DTS is the judgement */
	return sunxi_drv_prv.boot_disp ? true : false;
}

static int sunxi_drm_smooth_boot(struct drm_device *dev)
{
	int ret;

	struct sunxi_drm_crtc *scrtc;
	struct drm_encoder *enc;
	struct sunxi_drm_encoder *senc;
	struct sunxi_drm_connector *sconn;

	scrtc = sunxi_drm_crtc_get_crtc(0);
	sconn = sunxi_drm_connector_get_connector(0);

	enc = sconn->connector.encoder;
	if (!enc) {
		DRM_ERROR("crtc:0 has NO attached encoder\n");
		return -1;
	}
	senc = to_sunxi_encoder(enc);

	ret = sunxi_drm_smooth_set_state(dev);
	if (ret < 0) {
		DRM_ERROR("sunxi_drm_smooth_set_state failed\n");
		return -1;
	}

	scrtc->sw_enable(scrtc);
	senc->sw_enable(senc);

	return 0;
}

/* this func is used to decide bpp of fbdev fb */
unsigned int sunxi_drm_get_init_bpp(void)
{
#ifdef CONFIG_DRM_FBDEV_EMULATION
	if (sunxi_drv_prv.bootlogo.bpp)
		return sunxi_drv_prv.bootlogo.bpp;
#endif

	return 32;
}

/* this func is used to decide width of fbdev fb */
unsigned int sunxi_drm_get_init_width(void)
{
	unsigned int w, h, f;
	struct sunxi_drm_connector *sconn
			= sunxi_drm_connector_get_connector(0);

#ifdef CONFIG_DRM_FBDEV_EMULATION
	if (sunxi_drv_prv.bootlogo.width)
		return sunxi_drv_prv.bootlogo.width;
#endif

	sconn->get_init_resolution(sconn, &w, &h, &f);
	return w;
}

/* this func is used to decide height of fbdev fb */
unsigned int sunxi_drm_get_init_height(void)
{
	unsigned int w, h, f;
	struct sunxi_drm_connector *sconn
			= sunxi_drm_connector_get_connector(0);

#ifdef CONFIG_DRM_FBDEV_EMULATION
	if (sunxi_drv_prv.bootlogo.height)
		return sunxi_drv_prv.bootlogo.height;
#endif

	sconn->get_init_resolution(sconn, &w, &h, &f);
	return h;
}

unsigned int sunxi_drm_get_connector_type(int i)
{
	if (i >= sunxi_drv_prv.connector_num) {
		DRM_ERROR("connector:%d is out of range!\n", i);
		return 0;
	}

	return sunxi_drv_prv.connector_type[i];
}

char *sunxi_drm_get_connector_mode(int i)
{
	if (i >= sunxi_drv_prv.connector_num) {
		DRM_ERROR("connector:%d is out of range!\n", i);
		return NULL;
	}

	return (char *)&sunxi_drv_prv.connector_mode[i][0];
}

unsigned int sunxi_drm_get_connector_count(void)
{
	return sunxi_drv_prv.connector_num;
}

static int sunxi_drm_init(struct drm_device *dev)
{
	if (sunxi_drm_crtc_init(dev) < 0) {
		DRM_ERROR("sunxi_drm_crtc_init failed\n");
		goto crtc_err;
	}

	if (sunxi_drm_encoder_init(dev) < 0) {
		DRM_ERROR("sunxi_drm_encoder_init failed\n");
		goto enc_err;
	}

	if (sunxi_drm_connector_init(dev) < 0) {
		DRM_ERROR("sunxi_drm_connector_init failed\n");
		goto con_err;
	}

#ifdef CONFIG_DRM_FBDEV_EMULATION
	if (sunxi_drm_fbdev_init(dev) < 0) {
		DRM_ERROR("sunxi_drm_fbdev_init failed\n");
		goto con_err;
	}
#endif

	return 0;

con_err:
	sunxi_drm_connector_exit(dev);
enc_err:
	sunxi_drm_encoder_exit(dev);
crtc_err:
	sunxi_drm_crtc_exit(dev);
	DRM_ERROR("sunxi drm init failed\n");
	return -1;
}

void sunxi_drm_destroy(struct drm_device *dev)
{
#ifdef CONFIG_DRM_FBDEV_EMULATION
	sunxi_drm_fbdev_exit(dev);
#endif
	sunxi_drm_connector_exit(dev);
	sunxi_drm_encoder_exit(dev);
	sunxi_drm_crtc_exit(dev);
}

static void sunxi_drm_output_poll_changed(struct drm_device *dev)
{
#ifdef CONFIG_DRM_FBDEV_EMULATION
	sunxi_drm_fbdev_output_poll_changed(dev);
#else
	/* struct drm_crtc *crtc;
	struct drm_encoder *encoder;
	bool find = false;

	drm_for_each_crtc(crtc, dev) {
		if (!crtc->state)
			continue;
		if (crtc->state->active)
			continue;

		crtc->helper_private->enable(crtc);

		drm_for_each_encoder(encoder, dev) {
			if (encoder->crtc == crtc) {
				find = true;
				break;
			}
		}

		if (!find) {
			DRM_ERROR("can NOT find an \n");
		}
	} */
#endif
	return;
}

static const struct drm_mode_config_funcs sunxi_drm_mode_config_funcs = {
	.atomic_check		= drm_atomic_helper_check,
	.atomic_commit		= drm_atomic_helper_commit,
	.output_poll_changed	= sunxi_drm_output_poll_changed,
	.fb_create		= sunxi_drm_fb_create,
};

void sunxi_drm_mode_config_init(struct drm_device *dev)
{
	dev->mode_config.min_width = 0;
	dev->mode_config.min_height = 0;

	/* max_width be decided by the de bufferline */
	dev->mode_config.max_width = 8192;
	dev->mode_config.max_height = 8192;

	dev->mode_config.funcs = &sunxi_drm_mode_config_funcs;

}

static int sunxi_drm_init_iommu(struct drm_device *drm_dev)
{
	int ret;
	struct sunxi_drm_private *priv = drm_dev->dev_private;

	if (!IS_ENABLED(CONFIG_AW_IOMMU) || !IS_ENABLED(CONFIG_DRM_AW_IOMMU))
		return 0;

	/* create common IOMMU mapping for all devices attached to SUNXI DRM */
	ret = sunxi_drm_create_iommu_mapping(drm_dev);
	if (ret < 0) {
		DRM_ERROR("failed to create iommu mapping.\n");
		return ret;
	}

	return 0;
}

static int sunxi_drm_load(struct drm_device *dev, unsigned long flags)
{
	int ret;

	DRM_INFO("[DRM-DRV]%s start\n", __func__);

	dev->dev_private = (void *)&sunxi_drv_prv;
	sunxi_drv_prv.drm_dev = dev;

	if (sunxi_drm_init_iommu(dev)) {
		DRM_ERROR("sunxi_drm_init_iommu failed!\n");
		goto load_err;
	}

	drm_mode_config_init(dev);
	sunxi_drm_mode_config_init(dev);

	if (sunxi_drm_init(dev)) {
		DRM_ERROR("failed to initialize sunxi drm dev.\n");
		goto load_err;
	}

	dev->irq_enabled = 1;
	ret = drm_vblank_init(dev, dev->mode_config.num_crtc);
	if (ret) {
		DRM_ERROR("failed to init vblank.\n");
		goto load_err;
	}

	/* init kms poll for handling hpd */
	drm_kms_helper_poll_init(dev);
	return 0;

load_err:
	sunxi_drm_destroy(dev);
	drm_mode_config_cleanup(dev);
	sunxi_drv_prv.drm_dev = NULL;
	DRM_ERROR("LOAD failed\n");
	return -1;
}

static void sunxi_drm_unload(struct drm_device *dev)
{
	/* drm_vblank_cleanup(dev); */
	sunxi_drm_destroy(dev);
	drm_mode_config_cleanup(dev);
}

static struct drm_ioctl_desc sunxi_ioctls[] = {
	DRM_IOCTL_DEF_DRV(SUNXI_GEM_CREATE, sunxi_drm_gem_create_ioctl,
			DRM_AUTH | DRM_RENDER_ALLOW),
	DRM_IOCTL_DEF_DRV(SUNXI_SET_ENHANCE, sunxi_drm_crtc_set_enhance_ioctl,
			DRM_AUTH | DRM_RENDER_ALLOW),
	DRM_IOCTL_DEF_DRV(SUNXI_GET_ENHANCE, sunxi_drm_crtc_get_enhance_ioctl,
			DRM_AUTH | DRM_RENDER_ALLOW),
	DRM_IOCTL_DEF_DRV(SUNXI_SET_SMBL, sunxi_drm_crtc_set_smbl_ioctl,
			DRM_AUTH | DRM_RENDER_ALLOW),
	DRM_IOCTL_DEF_DRV(SUNXI_GET_SMBL, sunxi_drm_crtc_get_smbl_ioctl,
			DRM_AUTH | DRM_RENDER_ALLOW),
	DRM_IOCTL_DEF_DRV(SUNXI_GEM_FD_TO_PHYADDR, sunxi_drm_gem_get_phyaddr_ioctl,
			DRM_UNLOCKED),
};

static const struct file_operations sunxi_drm_driver_fops = {
	.owner = THIS_MODULE,
	.open = drm_open,
	.mmap = sunxi_drm_gem_mmap,
	.poll = drm_poll,
	.read = drm_read,
	.unlocked_ioctl	= drm_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = drm_compat_ioctl,
#endif
	.release = drm_release,
};

static struct drm_driver sunxi_drm_driver = {
	.driver_features = DRIVER_HAVE_IRQ | DRIVER_MODESET |
		DRIVER_GEM | DRIVER_ATOMIC,
	.ioctls = sunxi_ioctls,
	.fops = &sunxi_drm_driver_fops,
	.name = DRIVER_NAME,
	.desc = DRIVER_DESC,
	.date = DRIVER_DATE,
	.major = DRIVER_MAJOR,
	.minor = DRIVER_MINOR,
	.load = sunxi_drm_load,
	.unload = sunxi_drm_unload,

	/* gem */
	.gem_vm_ops = &sunxi_drm_gem_vm_ops,
	.gem_free_object_unlocked = sunxi_drm_gem_free_object,
	.dumb_create = sunxi_drm_gem_dumb_create,
	.dumb_map_offset = sunxi_drm_gem_dumb_map_offset,
	.dumb_destroy = drm_gem_dumb_destroy,

	/* prime */
	.prime_handle_to_fd = drm_gem_prime_handle_to_fd,
	.prime_fd_to_handle = drm_gem_prime_fd_to_handle,
	.gem_prime_export = drm_gem_prime_export,
	.gem_prime_import = drm_gem_prime_import,
	.gem_prime_get_sg_table = sunxi_drm_gem_prime_get_sg_table,
	.gem_prime_import_sg_table = sunxi_drm_gem_prime_import_sg_table,
	.gem_prime_mmap = sunxi_drm_gem_prime_mmap,

	/* vblank */
	.enable_vblank = sunxi_drm_enable_vblank,
	.disable_vblank = sunxi_drm_disable_vblank,
	.get_vblank_counter = sunxi_drm_vblank_counter,
};

static int sunxi_drm_parse_dts(struct device_node *np)
{
	int ret = 0, i;
	unsigned char string[30] = { 0 };
#ifdef CONFIG_DRM_FBDEV_EMULATION
	unsigned int boot_disp;
#endif

	/* read bootlogo physical address */
	ret = of_property_read_u32(np, "fb_base",
				&sunxi_drv_prv.fb_base);
	if (ret < 0) {
		DRM_ERROR("Parse DTS-info fb_base failed!\n");
		return ret;
	}

	if (sunxi_drv_prv.fb_base == 0) {
		const char *boot_fb;

		ret = of_property_read_string(np, "boot_fb0",
				&boot_fb);
		if (ret < 0) {
			DRM_ERROR("There is NO boot_fb0\n");
			return ret;
		}

		strcpy(sunxi_drv_prv.boot_fb, (char *)boot_fb);
	}

#ifdef CONFIG_DRM_FBDEV_EMULATION
	/* read that if there is a bootlogo output during bootloader */
	ret = of_property_read_u32(np, "boot_disp", &boot_disp);
	if (ret < 0) {
		DRM_ERROR("Parse DTS-info boot_disp failed!\n");
		return ret;
	}
	sunxi_drv_prv.boot_disp = boot_disp ? true : false;
#endif

	for (i = 0; i < MAX_CONNECTOR_COUNT; i++) {
		const char *mode_name;

		sprintf(string, "connector%d_output_type", i);
		ret = of_property_read_u32(np, string,
				&sunxi_drv_prv.connector_type[i]);
		if (ret < 0 || (!sunxi_drv_prv.connector_type[i])) {
			DRM_INFO("WARN:Parse DTS-info connector%d_output_type failed!\n",
					i);
			break;
		}

		sprintf(string, "connector%d_output_mode", i);
		ret = of_property_read_string(np, string, &mode_name);
		if (ret < 0) {
			DRM_ERROR("Parse DTS-info connector%d_output_mode failed!\n",
					i);
			break;
		}

		strcpy(sunxi_drv_prv.connector_mode[i], (char *)mode_name);
		/* memcpy(sunxi_drv_prv.connector_mode[i], mode_name, 20);

		DRM_INFO("Parse DTS-info:%s result:%s   - %c %c %c\n",
			string, sunxi_drv_prv.connector_mode[i],
			mode_name[0], mode_name[1], mode_name[2]); */

		sunxi_drv_prv.connector_num++;
	}

	DRM_DEBUG_DRIVER("SUNXI DRM DTS INFO:\n");
	DRM_DEBUG_DRIVER("fb_base:%u\n", sunxi_drv_prv.fb_base);
	DRM_DEBUG_DRIVER("boot_disp:%d\n", sunxi_drv_prv.boot_disp);
	DRM_DEBUG_DRIVER("connector_num:%d\n", sunxi_drv_prv.connector_num);
	for (i = 0; i < sunxi_drv_prv.connector_num; i++) {
		DRM_DEBUG_DRIVER("connector%d_type:%u\n",
					i, sunxi_drv_prv.connector_type[i]);
		DRM_DEBUG_DRIVER("connector%d_mode:%s\n",
					i, sunxi_drv_prv.connector_mode[i]);
	}

	return 0;
}

static int sunxi_drm_platform_probe(struct platform_device *pdev)
{
	int ret;
	struct device_node *np;

	DRM_INFO("[DRM-DRV]%s start\n", __func__);

	memset(&sunxi_drv_prv, 0, sizeof(sunxi_drv_prv));
	pdev->dev.coherent_dma_mask = DMA_BIT_MASK(64);
	sunxi_drm_driver.num_ioctls = ARRAY_SIZE(sunxi_ioctls);
	sunxi_drv_prv.pdev = pdev;
	np = pdev->dev.of_node;

	ret = sunxi_drm_parse_dts(np);
	if (ret < 0) {
		DRM_ERROR("sunxi_drm_parse_dts FAILED!\n");
		goto drm_err;
	}

#ifdef CONFIG_DRM_FBDEV_EMULATION
	/* parse bootlogo bmp file */
	if (sunxi_drv_prv.fb_base) {
		ret = sunxi_drm_parse_bmp_header(&sunxi_drv_prv.bootlogo_bmp_header,
							sunxi_drv_prv.fb_base);
		if (ret < 0) {
			DRM_INFO("parse bmp bootlogo failed, phy_addr:0x%x\n",
					sunxi_drv_prv.fb_base);
			goto drm_err;
		}

		sunxi_drm_get_bootlogoinfo_from_bmp_header(
				&sunxi_drv_prv.bootlogo,
				&sunxi_drv_prv.bootlogo_bmp_header,
				sunxi_drv_prv.fb_base);
	} else {
		sunxi_drm_get_bootlogoinfo_from_dts_info(&sunxi_drv_prv.bootlogo,
			sunxi_drv_prv.boot_fb);
	}
#endif

	ret = sunxi_drm_platform_init(&sunxi_drm_driver, &pdev->dev);
	if (ret < 0) {
		DRM_ERROR("sunxi_drm_platform_init\n");
		goto drm_err;
	}

#ifdef CONFIG_DRM_FBDEV_EMULATION
	if (sunxi_drv_prv.bootlogo.phy_addr) {
		ret = sunxi_drm_fbdev_copy_bootlogo(
			sunxi_drv_prv.drm_dev, &sunxi_drv_prv.bootlogo);
		if (ret < 0) {
			DRM_ERROR("fbdev copy bootlogo failed\n");
			goto drm_err;
		}
	}
#endif

	if (sunxi_drm_is_need_smooth_boot()) {
		if (sunxi_drm_smooth_boot(sunxi_drv_prv.drm_dev) < 0) {
			DRM_ERROR("sunxi_drm smooth booting failed\n");
			goto drm_err;
		}
	}

	DRM_INFO("[DRM-DRV]%s end\n", __func__);
	return 0;

drm_err:
	DRM_INFO("[DRM-DRV]%s failed\n", __func__);
	return -1;
}

static int sunxi_drm_platform_remove(struct platform_device *pdev)
{
	DRM_DEBUG_DRIVER("[%d]\n", __LINE__);

	drm_put_dev(dev_get_drvdata(&pdev->dev));

	return 0;
}

#ifdef CONFIG_PM_RUNTIME
static int sunxi_runtime_suspend(struct device *dev)
{
	return 0;
}

static int sunxi_runtime_resume(struct device *dev)
{
	return 0;
}

static int sunxi_runtime_idle(struct device *dev)
{
	return 0;
}
#endif

static int sunxi_drm_suspend(struct device *dev)
{
	struct drm_device *device = sunxi_drv_prv.drm_dev;
	struct drm_connector *conct = NULL;

	if ((dev != device->dev)
		|| (device == NULL)) {
		DRM_ERROR("devices are different or drm_device is NULL\n");
		return 0;
	}

	drm_modeset_lock_all(device);
	list_for_each_entry(conct, &device->mode_config.connector_list, head)
		conct->funcs->dpms(conct, DRM_MODE_DPMS_OFF);
	drm_modeset_unlock_all(device);

	return 0;
}

static int sunxi_drm_resume(struct device *dev)
{
	struct drm_device *device = sunxi_drv_prv.drm_dev;
	struct drm_connector *conct = NULL;

	if ((dev != device->dev)
		|| (device == NULL)) {
		DRM_ERROR("devices are different or drm_device is NULL\n");
		return 0;
	}


	drm_modeset_lock_all(device);
	list_for_each_entry(conct, &device->mode_config.connector_list, head)
		conct->funcs->dpms(conct, DRM_MODE_DPMS_ON);
	drm_modeset_unlock_all(device);

	return 0;
}


static const struct dev_pm_ops sunxi_drm_runtime_pm_ops = {
#ifdef CONFIG_PM_RUNTIME
	.runtime_suspend = sunxi_runtime_suspend,
	.runtime_resume = sunxi_runtime_resume,
	.runtime_idle = sunxi_runtime_idle,
#endif
	.suspend = sunxi_drm_suspend,
	.resume = sunxi_drm_resume,
};

static const struct of_device_id sunxi_of_match[] = {
	{ .compatible = "allwinner,sunxi-drm", },
	{},
};
MODULE_DEVICE_TABLE(of, sunxi_of_match);

static struct platform_driver sunxi_drm_platform_driver = {
	.probe = sunxi_drm_platform_probe,
	.remove = sunxi_drm_platform_remove,
	.driver = {
		.owner = THIS_MODULE,
		.name = "sunxi-drm",
		.of_match_table = sunxi_of_match,
		.pm = &sunxi_drm_runtime_pm_ops,
	},
};


static int __init sunxi_drm_drv_init(void)
{
	int ret;

	DRM_INFO("[DRM-DRV]%s start\n\n", __func__);
/* register sunxi-de driver */
	ret = sunxi_de_module_init();
	if (ret < 0) {
		DRM_ERROR("sunxi_de_module_init failed!\n");
		goto de_err;
	}

/* register sunxi-tcon driver */
	ret = sunxi_tcon_module_init();
	if (ret < 0) {
		DRM_ERROR("sunxi_tcon_module_init failed!\n");
		goto tcon_err;
	}
/* register sunxi-lcd driver */
#ifdef CONFIG_AW_DRM_LCD
	ret = sunxi_lcd_module_init();
	if (ret < 0) {
		sunxi_lcd_module_exit();
		DRM_ERROR("sunxi_lcd_module_init failed!\n");
		goto drm_err;
	}
#endif

#ifdef CONFIG_AW_DRM_TV
	ret = sunxi_tv_module_init();
	if (ret < 0) {
		DRM_ERROR("sunxi_tv_module_init failed!\n");
		sunxi_tv_module_exit();
		goto drm_err;
	}
#endif

#if defined(CONFIG_AW_DRM_HDMI14)
	ret = sunxi_hdmi_module_init();
	if (ret < 0) {
		DRM_ERROR("sunxi_hdmi_module_init failed!\n");
		sunxi_hdmi_module_exit();
		goto drm_err;
	}
#elif defined(CONFIG_AW_DRM_HDMI20)
	ret = sunxi_hdmi20_module_init();
	if (ret < 0) {
		DRM_ERROR("sunxi_hdmi_module_init failed!\n");
		sunxi_hdmi20_module_exit();
		goto drm_err;
	}
#endif

/* register sunxi-drm driver */
	ret = platform_driver_register(&sunxi_drm_platform_driver);
	if (ret < 0) {
		DRM_ERROR("drm platform_driver_register failed!\n");
		goto drm_err;
	}

	/* if (sunxi_drv_prv.drm_dev) {
		if (sunxi_drm_sysfs_init(sunxi_drv_prv.drm_dev) < 0) {
			DRM_ERROR("sunxi_drm_sysfs_init failed\n");
			goto drm_err;
		}
	} */

	DRM_INFO("[DRM-DRV]%s end\n\n", __func__);
	return 0;

drm_err:
	platform_driver_unregister(&sunxi_drm_platform_driver);

tcon_err:
	sunxi_tcon_module_exit();
de_err:
	sunxi_de_module_exit();
	DRM_INFO("[DRM-DRV]%s failed\n", __func__);
	return -1;
}

static void __exit sunxi_drm_drv_exit(void)
{
	DRM_INFO("[DRM-DRV]%s start\n", __func__);

	/* sunxi_drm_sysfs_exit(sunxi_drv_prv.drm_dev); */
	platform_driver_unregister(&sunxi_drm_platform_driver);
#ifdef CONFIG_AW_DRM_LCD
	sunxi_lcd_module_exit();
#endif

#ifdef CONFIG_AW_DRM_TV
	sunxi_tv_module_exit();
#endif

#if defined(CONFIG_AW_DRM_HDMI14)
	sunxi_hdmi_module_exit();
#elif defined(CONFIG_AW_DRM_HDMI20)
	sunxi_hdmi20_module_exit();
#endif
	sunxi_tcon_module_exit();
	sunxi_de_module_exit();
}

module_init(sunxi_drm_drv_init);
module_exit(sunxi_drm_drv_exit);
MODULE_AUTHOR("zhengwanyu");
MODULE_AUTHOR("hongyaobin");

MODULE_DESCRIPTION("Allwinnertech SoC DRM Driver");
MODULE_LICENSE("GPL");
