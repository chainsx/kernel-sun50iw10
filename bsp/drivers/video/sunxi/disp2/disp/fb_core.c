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

/* double buffer */
#define FB_BUFFER_CNT		2
#define FBIOPAN_DISPLAY_SUNXI	0x4650

static struct fb_info **fb_infos;

static int sunxi_fb_release(struct fb_info *info, int user)
{
	return 0;
}

static int sunxi_fb_open(struct fb_info *info, int user)
{
	return 0;
}

static int sunxi_fb_pan_display(struct fb_var_screeninfo *var,
				struct fb_info *info)
{
	fb_debug_inf("fb %d pan display start update\n", info->node);
	platform_update_fb_output(info->par, var);
	fb_debug_inf("fb %d pan display update ok\n", info->node);
	platform_fb_pan_display_post_proc(info->par);
	fb_debug_inf("fb %d pan display ok\n", info->node);
	return 0;
}

static int sunxi_fb_ioctl(struct fb_info *info, unsigned int cmd,
			  unsigned long arg)
{
	int ret = 0;
	struct fb_var_screeninfo var;
	void __user *argp = (void __user *)arg;
	switch (cmd) {
	case FBIOPAN_DISPLAY_SUNXI:
		if (copy_from_user(&var, argp, sizeof(var)))
			return -EFAULT;
		ret = sunxi_fb_pan_display(&var, info);
		break;
	default:
		ret = -EINVAL;
		break;
	}
	return ret;
}

static int sunxi_fb_check_var(struct fb_var_screeninfo *var,
			      struct fb_info *info)
{
	platform_fb_check_rotate(&var->rotate);
	return 0;
}

static int sunxi_fb_set_par(struct fb_info *info)
{
	fb_debug_inf("fb %d set rot %d\n", info->node, info->var.rotate);
	return platform_fb_set_rotate(info->par, info->var.rotate);
}

void test_rot(int rot)
{
	struct fb_var_screeninfo tmp;
	memcpy(&tmp, &fb_infos[0]->var, sizeof(tmp));
	tmp.rotate = rot;

	sunxi_fb_check_var(&tmp, fb_infos[0]);
	memcpy(&fb_infos[0]->var, &tmp, sizeof(tmp));
	sunxi_fb_set_par(fb_infos[0]);
}

static int sunxi_fb_blank(int blank_mode, struct fb_info *info)
{
	if (!(blank_mode == FB_BLANK_POWERDOWN ||
		blank_mode == FB_BLANK_UNBLANK))
		return -1;
	return platform_fb_set_blank(info->par,
					blank_mode == FB_BLANK_POWERDOWN ?
						1 : 0);
}

static int sunxi_fb_mmap(struct fb_info *info, struct vm_area_struct *vma)
{
	return platform_fb_mmap(info->par, vma);
}

static struct fb_ops dispfb_ops = {
	.owner = THIS_MODULE,
	.fb_open = sunxi_fb_open,
	.fb_release = sunxi_fb_release,
	.fb_pan_display = sunxi_fb_pan_display,
#if defined(CONFIG_COMPAT)
	.fb_compat_ioctl = sunxi_fb_ioctl,
#endif
	.fb_ioctl = sunxi_fb_ioctl,
	.fb_check_var = sunxi_fb_check_var,
	.fb_set_par = sunxi_fb_set_par,
	.fb_blank = sunxi_fb_blank,
	.fb_mmap = sunxi_fb_mmap,
};

static int fb_init_var(void *hw_info, struct fb_var_screeninfo *var, enum disp_pixel_format format, u32 fb_width, u32 fb_height)
{
	var->nonstd = 0;
	var->xoffset = 0;
	var->yoffset = 0;
	var->xres = fb_width;
	var->yres = fb_height;
	var->xres_virtual = fb_width;
	var->yres_virtual = fb_height * FB_BUFFER_CNT;
	var->activate = FB_ACTIVATE_FORCE;

	platform_format_to_var(format, var);
	platform_get_timing(hw_info, var);
	platform_get_physical_size(hw_info, var);
	return 0;
}

static int fb_init_fix(struct fb_fix_screeninfo *fix, unsigned long device_addr, int buffer_width, int buffer_height)
{
	fix->line_length = buffer_width;
	fix->smem_len = buffer_width * buffer_height;
	fix->smem_start = device_addr;
	fix->type_aux = 0;
	fix->xpanstep = 1;
	fix->ypanstep = 1;
	fix->ywrapstep = 0;
	fix->mmio_start = 0;
	fix->mmio_len = 0;
	fix->accel = FB_ACCEL_NONE;
	fix->type = FB_TYPE_PACKED_PIXELS;
	fix->visual = FB_VISUAL_TRUECOLOR;
	return 0;
}

/* for gpu usage */
s32 sunxi_get_fb_addr_para(struct __fb_addr_para *fb_addr_para)
{
	if (fb_addr_para && fb_infos[0]) {
		fb_addr_para->fb_paddr = fb_infos[0]->fix.smem_start;
		fb_addr_para->fb_size = fb_infos[0]->fix.smem_len;
		return 0;
	}
	return -1;
}
EXPORT_SYMBOL(sunxi_get_fb_addr_para);

int fb_core_init(struct fb_create_info *create)
{
	int i;
	int bpp;
	int ret = 0;
	unsigned int height;
	unsigned int width;
	char *virtual_addr;
	struct fb_info *info;
	unsigned long long device_addr;
	platform_fb_init(&create->fb_share);
	bpp = platform_format_get_bpp(create->fb_share.format);
	fb_infos = kmalloc(sizeof(*fb_infos) * create->fb_share.fb_num, GFP_KERNEL | __GFP_ZERO);

	for (i = 0; i < create->fb_share.fb_num; i++) {
		info = framebuffer_alloc(platform_get_private_size(), create->fb_share.disp_dev);
		fb_infos[i] = info;
		width = create->fb_each[i].width;
		height = create->fb_each[i].height;
		platform_fb_init_for_each(info->par, &create->fb_each[i], i);
		ret = platform_fb_memory_alloc(info->par, &virtual_addr, &device_addr,
						width * height * FB_BUFFER_CNT * bpp / 8);
		info->screen_base = virtual_addr;
		info->fbops = &dispfb_ops;
		info->flags = 0;
		fb_init_var(info->par, &info->var, create->fb_share.format, width, height);
		fb_init_fix(&info->fix, device_addr, width * info->var.bits_per_pixel / 8, height * FB_BUFFER_CNT);
		register_framebuffer(info);
		platform_fb_g2d_rot_init(info->par);
		platform_fb_init_logo(info->par, &info->var);
		fb_debug_inf("fb %d vir 0x%p phy 0x%8llx\n", i, virtual_addr, device_addr);
	}
	platform_fb_post_init();
	return ret;
}

int fb_core_exit(struct fb_create_info *create)
{
	int i;
	fb_debug_inf("%s start\n", __FUNCTION__);
	platform_fb_exit();
	for (i = 0; i < create->fb_share.fb_num; i++) {
		platform_fb_g2d_rot_exit(fb_infos[i]->par);
		platform_fb_exit_for_each(fb_infos[i]->par);
		unregister_framebuffer(fb_infos[i]);
		framebuffer_release(fb_infos[i]);
	}
	platform_fb_post_exit();
	kfree(fb_infos);
	return 0;
}
