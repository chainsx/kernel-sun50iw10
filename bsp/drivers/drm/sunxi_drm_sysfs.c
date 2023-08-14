/*
 * Copyright (C) 2019 Allwinnertech Co.Ltd
 * Authors: zhengwanyu
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>

#include "sunxi_drm_fb.h"
#include "sunxi_drm_plane.h"
#include "sunxi_drm_gem.h"
#include "sunxi_drm_crtc.h"
#include "sunxi_drm_encoder.h"
#include "sunxi_drm_connector.h"
#include "sunxi_drm_sysfs.h"
#include "sunxi_drm_drv.h"

#if DRM_DEBUG
struct device *sunxi_drv_dev;

static ssize_t sunxi_planes_info_show(struct device *device,
			   struct device_attribute *attr,
			   char *buf)
{
	ssize_t n = 0;
	struct drm_device *drm_dev = sunxi_drm_get_drm_device();

	n += sunxi_drm_planes_show(buf + n, drm_dev, NULL);

	return n;
}

static ssize_t sunxi_planes_info_store(struct device *device,
			   struct device_attribute *attr,
			   const char *buf, size_t count)
{
	return count;
}

static DEVICE_ATTR(planes_info, 0660, sunxi_planes_info_show, sunxi_planes_info_store);


static ssize_t sunxi_crtc_info_show(struct device *device,
			   struct device_attribute *attr,
			   char *buf)
{
	ssize_t n = 0;
	struct drm_device *drm_dev = sunxi_drm_get_drm_device();

	n += sunxi_drm_crtc_show(buf + n, drm_dev);

	return n;
}

static ssize_t sunxi_crtc_info_store(struct device *device,
			   struct device_attribute *attr,
			   const char *buf, size_t count)
{
	return count;
}

static DEVICE_ATTR(crtc_info, 0660, sunxi_crtc_info_show, sunxi_crtc_info_store);


static unsigned int fb_id;

static char sunxi_printable_char(int c)
{
	return isascii(c) && isprint(c) ? c : '?';
}

char *sunxi_drm_get_path_name(char *dir, uint32_t format,
						uint32_t w, uint32_t h)
{
	char *buf = kmalloc(100, GFP_KERNEL);

	snprintf(buf, 32,
		 "%s/%dx%d_%c%c%c%c",
		 dir, w, h,
		 sunxi_printable_char(format & 0xff),
		 sunxi_printable_char((format >> 8) & 0xff),
		 sunxi_printable_char((format >> 16) & 0xff),
		 sunxi_printable_char((format >> 24) & 0x7f));

	return buf;
}

static ssize_t sunxi_fb_capture_show(struct device *device,
			   struct device_attribute *attr,
			   char *buf)
{
	ssize_t n = 0;
	unsigned int i;
	bool find = false;
	struct drm_framebuffer *fb = NULL;
	struct sunxi_drm_fb *sunxi_fb = NULL;
	unsigned char *vaddr;

	struct file *fp;
	mm_segment_t fs;
	loff_t pos;

	char *path;
	struct drm_device *dev = sunxi_drm_get_drm_device();

	mutex_lock(&dev->mode_config.fb_lock);
	drm_for_each_fb(fb, dev) {
		DRM_INFO("fb base id:%d\n", fb->base.id);
		if (fb->base.id == fb_id) {
			find = true;
			break;
		}
	}
	mutex_unlock(&dev->mode_config.fb_lock);

	if (!find) {
		DRM_ERROR("[ERROR]fb_id:%d is invalid!\n", fb_id);
		return n;
	}

	sunxi_fb = to_sunxi_fb(fb);

	path = sunxi_drm_get_path_name("/mnt", fb->pixel_format,
					fb->width, fb->height);
	if ((fb->pixel_format == DRM_FORMAT_XRGB8888)
			|| (fb->pixel_format == DRM_FORMAT_ARGB8888)) {
		fp = filp_open(path, O_RDWR | O_CREAT, 0644);
		if (IS_ERR(fp)) {
			n += sprintf(buf + n, "create file error\n");
			kfree(path);
			return n;
		}

		fs = get_fs();
		set_fs(KERNEL_DS);

		vaddr = sunxi_fb->obj[0]->vaddr;

		n += sprintf(buf + n, "Wait for capture!\n");
		pos = fp->f_pos;
		vfs_write(fp, vaddr, fb->pitches[0] * fb->height, &pos);
		n += sprintf(buf + n, "Capture finished!\n");

		set_fs(fs);
		filp_close(fp, NULL);
	} else if (fb->pixel_format == DRM_FORMAT_RGB888) {
		unsigned char alpha = 0xff;

		fp = filp_open(path, O_RDWR | O_CREAT, 0644);
		if (IS_ERR(fp)) {
			n += sprintf(buf + n, "create file error\n");
			kfree(path);
			return n;
		}

		vaddr = sunxi_fb->obj[0]->vaddr;

		fs = get_fs();
		set_fs(KERNEL_DS);

		n += sprintf(buf + n, "Wait for capture, for 0~2 minute...\n");
		pos = fp->f_pos;
		for (i = 0; i < fb->width * fb->height; i++) {
			vfs_write(fp, &vaddr[3 * i], 3, &pos);
			vfs_write(fp, &alpha, 1, &pos);
		}
		n += sprintf(buf + n, "Capture finished!\n");

		set_fs(fs);
		filp_close(fp, NULL);
	} else {
		n += sprintf(buf + n, "[ERROR]NOT support format:%c%c%c%c\n",
				(char)(fb->pixel_format & 0xff),
				(char)((fb->pixel_format >> 8) & 0xff),
				(char)((fb->pixel_format >> 16) & 0xff),
				(char)((fb->pixel_format >> 24) & 0xff));
	}

	n += sprintf(buf + n, "Program Finished!\n");
	kfree(path);
	return n;
}

static ssize_t sunxi_fb_capture_store(struct device *device,
			   struct device_attribute *attr,
			   const char *buf, size_t count)
{
	fb_id = simple_strtoul(buf, NULL, 0);

	return count;
}

static DEVICE_ATTR(fb_capture, 0660, sunxi_fb_capture_show,
				sunxi_fb_capture_store);


static ssize_t sunxi_drm_fb_info_dump(char *buf)
{
	ssize_t n = 0;
	struct drm_framebuffer *fb;
	struct drm_device *dev = sunxi_drm_get_drm_device();

	mutex_lock(&dev->mode_config.fb_lock);
	n += sprintf(buf + n, "fb num:%d\n", dev->mode_config.num_fb);
	drm_for_each_fb(fb, dev)
		n += sunxi_drm_fb_show(buf + n, fb);
	mutex_unlock(&dev->mode_config.fb_lock);

	return n;
}

static ssize_t sunxi_fb_info_show(struct device *device,
			   struct device_attribute *attr,
			   char *buf)
{
	ssize_t n = 0;

	n += sunxi_drm_fb_info_dump(buf + n);

	return n;
}

static ssize_t sunxi_fb_info_store(struct device *device,
			   struct device_attribute *attr,
			   const char *buf, size_t count)
{
	return count;
}

static DEVICE_ATTR(fb_info, 0660, sunxi_fb_info_show, sunxi_fb_info_store);

static ssize_t sunxi_encoder_info_show(struct device *device,
			   struct device_attribute *attr,
			   char *buf)
{
	ssize_t n = 0;
	struct drm_device *drm_dev = sunxi_drm_get_drm_device();

	n += sunxi_drm_encoder_show(buf + n, drm_dev);

	return n;
}

static ssize_t sunxi_encoder_info_store(struct device *device,
			   struct device_attribute *attr,
			   const char *buf, size_t count)
{
	return count;
}

static DEVICE_ATTR(encoder_info, 0660, sunxi_encoder_info_show, sunxi_encoder_info_store);


static ssize_t sunxi_connector_info_show(struct device *device,
			   struct device_attribute *attr,
			   char *buf)
{
	ssize_t n = 0;
	struct drm_device *drm_dev = sunxi_drm_get_drm_device();

	n += sunxi_drm_connector_show(buf + n, drm_dev);

	return n;
}

static ssize_t sunxi_connector_info_store(struct device *device,
			   struct device_attribute *attr,
			   const char *buf, size_t count)
{
	return count;
}

static DEVICE_ATTR(connector_info, 0660, sunxi_connector_info_show, sunxi_connector_info_store);


static ssize_t sunxi_drm_debug_show(struct device *device,
			   struct device_attribute *attr,
			   char *buf)
{
	ssize_t n = 0;

	n += sprintf(buf + n, "drm_debug=%u\n\n", drm_debug);
	n += sprintf(buf + n, "params explain:\n");
	n += sprintf(buf + n, "NONE print:0x00\n");
	n += sprintf(buf + n, "CORE print:0x01\n");
	n += sprintf(buf + n, "DRIVER print:0x02\n");
	n += sprintf(buf + n, "KMS print:0x04\n");
	n += sprintf(buf + n, "PRIME print:0x08\n");
	n += sprintf(buf + n, "ATOMIC print:0x10\n");
	n += sprintf(buf + n, "VBL print:0x20\n");

	return n;
}

static ssize_t sunxi_drm_debug_store(struct device *device,
			   struct device_attribute *attr,
			   const char *buf, size_t count)
{
	if (kstrtoul(buf, 0, (unsigned long *)&drm_debug) < 0) {
		DRM_ERROR("ERROR input\n");
		return 0;
	}

	return count;
}

static DEVICE_ATTR(debug, 0660, sunxi_drm_debug_show, sunxi_drm_debug_store);

static struct attribute *sunxi_dev_attrs[] = {
	&dev_attr_planes_info.attr,
	&dev_attr_crtc_info.attr,
	&dev_attr_fb_capture.attr,
	&dev_attr_fb_info.attr,
	&dev_attr_encoder_info.attr,
	&dev_attr_connector_info.attr,
	&dev_attr_debug.attr,
	NULL
};

static const struct attribute_group sunxi_dev_group = {
	.attrs = sunxi_dev_attrs,
};

static const struct attribute_group *sunxi_drm_dev_groups[] = {
	&sunxi_dev_group,
	NULL
};

int sunxi_drm_sysfs_init(struct drm_device *dev)
{
	sunxi_drv_dev = device_create_with_groups(dev->primary->kdev->class,
						dev->dev, 0, dev,
						sunxi_drm_dev_groups,
						"driver");
	if (IS_ERR(sunxi_drv_dev)) {
		DRM_ERROR("failed to register connector device: %ld\n",
						PTR_ERR(sunxi_drv_dev));
		return -1;
	}

	return 0;
}


void sunxi_drm_sysfs_exit(struct drm_device *dev)
{
	device_unregister(sunxi_drv_dev);
}
#endif

