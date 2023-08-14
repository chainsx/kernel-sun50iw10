/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2022 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include "fb_top.h"
#include "fb_platform.h"

int fb_debug_val;
static struct fb_create_info create_info;

int fb_core_init(struct fb_create_info *create);
int fb_core_exit(struct fb_create_info *create);

int fb_parse_bootlogo_base(phys_addr_t *fb_base)
{
	*fb_base = (phys_addr_t) disp_boot_para_parse("fb_base");

	return 0;
}

static int get_boot_fb_info(struct logo_info_fb *fb)
{
	char *boot_fb_str = NULL;
	int ret;
	int i = 0;
	int len;
	char *tmp;
	char *p;
	void *buf[] = {
		&fb->phy_addr,
		&fb->width,
		&fb->height,
		&fb->bpp,
		&fb->stride,
		&fb->crop_l,
		&fb->crop_t,
		&fb->crop_r,
		&fb->crop_b,
	};

	boot_fb_str = (char *)disp_boot_para_parse_str("boot_fb0");
	if (!boot_fb_str) {
		pr_warn("parse boot_fb0 fail! NOT found\n");
		return -1;
	}

	len = strlen(boot_fb_str);
	tmp = kzalloc(len + 1, GFP_KERNEL);
	memcpy((void *)tmp, (void *)boot_fb_str, len);
	boot_fb_str = tmp;

	do {
		p = strstr(boot_fb_str, ",");
		if (p != NULL)
			*p = '\0';
		ret = kstrtoul(boot_fb_str, 16, buf[i]);
		if (ret)
			pr_warn("parse boot_fb fail! index = %d\n", i);
		i++;
		boot_fb_str = p + 1;
	} while (i < sizeof(buf) / sizeof(buf[0]) && p != NULL);

	kfree(tmp);
	fb_debug_inf("boot_fb para: src[phy_addr=%llx,w=%lu,h=%lu,bpp=%lu,stride=%lu,ltrb=%lu %lu %lu %lu]\n",
	     fb->phy_addr, fb->width, fb->height, fb->bpp, fb->stride,
	     fb->crop_l, fb->crop_t, fb->crop_r, fb->crop_b);

	return 0;
}

static int fb_get_logo_info(struct fb_create_info *info)
{
	get_boot_fb_info(&info->fb_share.logo.logo_fb);
	info->fb_share.logo_type = LOGO_FB;
	/* TODO add logo_file */
	return 0;
}

static bool is_fb_map_valid(struct fb_map *map)
{
	if (map->hw_display > bsp_disp_feat_get_num_screens())
		return false;
	if (map->hw_channel > bsp_disp_feat_get_num_channels(map->hw_display))
		return false;
	if (map->hw_layer > bsp_disp_feat_get_num_layers_by_chn(map->hw_display, map->hw_channel))
		return false;
	return true;
}

static int get_hw_rot_config(int fb_num, struct fb_create_info *info)
{
	int ret;
	unsigned int value = 0;
	char sub_name[32] = {0};
	int i = 0;

	for (i = 0; i < fb_num; i++) {
		info->fb_each[i].g2d_rot_used = false;
		sprintf(sub_name, "fb%d_rot_used", i);
		ret = disp_sys_script_get_item("disp", sub_name, &value, 1);
		if (!(ret == 1 && value == 1)) {
			continue;
		}
		sprintf(sub_name, "fb%d_rot_degree", i);
		ret = disp_sys_script_get_item("disp", sub_name, &value, 1);
		if (ret == 1) {
			info->fb_each[i].g2d_rot_used = true;
			info->fb_each[i].rot_degree = value;
		}

	}

	for (i = 0; i < fb_num; i++) {
		fb_debug_inf("fb_num %d use rot %d degree%d\n", i,
				info->fb_each[i].g2d_rot_used,
				info->fb_each[i].rot_degree);
	}
	return 0;
}

static int sunxi_get_fb_config(struct device *dev, struct fb_create_info *info)
{
	int i;
	int hw_id;
	u32 map[4] = {0};
	char name[32] = {0};
	unsigned int output_mode;
	bool is_logo[MAX_DEVICE] = {0};
	enum disp_output_type output_type;

	info->fb_share.disp_dev = dev;
	fb_debug_val = disp_boot_para_parse("fb_debug");
	if (fb_debug_val <= 0)
		fb_debug_val = 0;

	info->fb_share.fb_num = disp_boot_para_parse("fb_num");
	info->fb_share.format = disp_boot_para_parse("fb_format");

	fb_debug_inf("fb_num %d fb_format %d\n", info->fb_share.fb_num, info->fb_share.format);
	if (info->fb_share.fb_num <= 0)
		return -1;

	info->fb_each = kzalloc(sizeof(*info->fb_each) * info->fb_share.fb_num, GFP_KERNEL);
	for (i = 0; i < info->fb_share.fb_num; i++) {
		sprintf(name, "fb%d_map", i);
		disp_boot_para_parse_array(name, map, 4);
		info->fb_each[i].map.hw_display = map[0];
		info->fb_each[i].map.hw_channel = map[1];
		info->fb_each[i].map.hw_layer = map[2];
		info->fb_each[i].map.hw_zorder = map[3];

		if (!is_fb_map_valid(&info->fb_each[i].map)) {
			fb_debug_inf("fbinfo: fb: %d, disp: %d channel: %d layer: %d invalid\n",
				i, map[0], map[1], map[2]);
			return -1;
		}

		sprintf(name, "fb%d_width", i);
		info->fb_each[i].width = disp_boot_para_parse(name);
		sprintf(name, "fb%d_height", i);
		info->fb_each[i].height = disp_boot_para_parse(name);

		hw_id = info->fb_each[i].map.hw_display;
		if (info->fb_each[i].width == 0 &&
		      info->fb_each[i].height == 0) {
			output_type = g_disp_drv.disp_init.output_type[hw_id];
			output_mode = g_disp_drv.disp_init.output_mode[hw_id];
			platform_get_fb_buffer_size_from_config(hw_id,
								output_type,
								output_mode,
								&info->fb_each[i].width,
								&info->fb_each[i].height);
		}
		/* display logo on the first found fb for every output */
		if (!is_logo[hw_id]) {
			is_logo[hw_id] = true;
			info->fb_each[i].logo_display = true;
		}
		fb_debug_inf("fbinfo: fb: %d, disp: %d channel: %d layer: %d zorder: %d w: %d h: %d\n",
			i, map[0], map[1], map[2], map[3], info->fb_each[i].width, info->fb_each[i].height);

	}

	info->fb_share.smooth_display = g_disp_drv.para.boot_info.sync;
	info->fb_share.smooth_display_hw_id = g_disp_drv.para.boot_info.disp;

	memcpy(info->fb_share.output_type, g_disp_drv.disp_init.output_type, sizeof(*info->fb_share.output_type) * MAX_DEVICE);
	memcpy(info->fb_share.output_mode, g_disp_drv.disp_init.output_mode, sizeof(*info->fb_share.output_mode) * MAX_DEVICE);
	fb_get_logo_info(info);

	get_hw_rot_config(info->fb_share.fb_num, info);
	fb_debug_inf("smooth display:%d hw_id %d disp0: type%d mode%d disp1: type%d mode%d\n",
		info->fb_share.smooth_display, info->fb_share.smooth_display_hw_id,
		info->fb_share.output_type[0], info->fb_share.output_mode[0],
		info->fb_share.output_type[1], info->fb_share.output_mode[1]);
	return 0;
}

s32 fb_init(struct platform_device *pdev)
{
	int ret;
	ret = sunxi_get_fb_config(&pdev->dev, &create_info);
	if (ret)
		goto OUT;
	ret = fb_core_init(&create_info);
OUT:
	return ret;
}

s32 fb_exit(void)
{
	fb_core_exit(&create_info);
	memset(&create_info, 0, sizeof(create_info));
	return 0;
}
