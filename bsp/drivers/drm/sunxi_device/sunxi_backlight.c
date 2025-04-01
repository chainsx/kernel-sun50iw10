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
#include <drm/drm_print.h>
#include "disp_sys_intf.h"
#include "sunxi_common.h"
#include "sunxi_backlight.h"

static unsigned int sunxi_backlight_cnt;
static struct sunxi_lcd_backlight *sunxi_backlight[BL_NUM_MAX];

static struct sunxi_lcd_backlight *
sunxi_backlight_get_backlight(unsigned int lcd_id)
{
	if (lcd_id < BL_NUM_MAX)
		return sunxi_backlight[lcd_id];
	return NULL;
}

ssize_t sunxi_backlight_info(char *buf)
{
	ssize_t n = 0;
	unsigned int i, j;
	struct sunxi_lcd_backlight *bl;

	n += sprintf(buf+n, "backlight count:%u\n\n", sunxi_backlight_cnt);
	for (i = 0; i < BL_NUM_MAX; i++) {
		bl = sunxi_backlight_get_backlight(i);
		n += sprintf(buf + n, "is LCD:%u backlight\n", i);
		n += sprintf(buf + n, "bl_en info:\n");
		n += sprintf(buf + n, "use_bl_en:%u\n", bl->use_bl_en);
		n += sprintf(buf + n, "gpio setting, name:%s gpio:%d\n",
			bl->bl_en_gpio.name, bl->bl_en_gpio.gpio);
		n += sprintf(buf + n, "bl_gpio_hdl:%d\n", bl->bl_gpio_hdl);
		n += sprintf(buf + n, "bl_en_power:%s\n", bl->bl_en_power);
		n += sprintf(buf + n, "bl_en_count:%u\n\n", bl->bl_en_count);

		n += sprintf(buf + n, "use_pwm:%u\n", bl->use_pwm);
		n += sprintf(buf + n, "bright:%llu\n", bl->bright);
		n += sprintf(buf + n, "pwm_info:chn%u pol:%u period_ns:%u"
			" duty_ns:%u en:%u\n", bl->pwm_info.channel,
			bl->pwm_info.polarity, bl->pwm_info.period_ns,
			bl->pwm_info.duty_ns, bl->pwm_info.enabled);
		n += sprintf(buf + n, "diming:%u\n", bl->dimming);
		n += sprintf(buf + n, "bright_curve_tbl:\n");
		for (j = 0; j < 256; j++) {
			if ((j != 0) && !(j % 10))
				n += sprintf(buf + n, "\n");
			n += sprintf(buf + n, "%02x ", bl->bright_curve_tbl[j]);
		}
		n += sprintf(buf + n, "\n");
	}

	return n;
}

/* To check if there is a backlight that contain a same pwm */
struct pwm_info *sunxi_backlight_get_same_pwm(unsigned int lcd_id,
						unsigned int pwm_ch)
{
	int i;
	struct sunxi_lcd_backlight *bl;

	if (lcd_id >= BL_NUM_MAX)
		return NULL;

	for (i = 0; i < lcd_id; i++) {
		bl = sunxi_backlight_get_backlight(i);
		if (bl->pwm_info.channel == pwm_ch)
			return &bl->pwm_info;
	}

	return NULL;
}

static int sunxi_backlight_set_bright(unsigned int lcd_id,
				unsigned int bright)
{
	int ret = 0;
	unsigned int bl_value, brightness;
	struct sunxi_lcd_backlight *bl = NULL;

	bl = sunxi_backlight_get_backlight(lcd_id);
	if (!bl) {
		DRM_ERROR("Null bl pointer!\n");
		return -1;
	}

	brightness = (bright > 255) ? 255 : bright;

		/* need to implement SMBL here */

#if IS_ENABLED(CONFIG_AW_PWM) || IS_ENABLED(CONFIG_PWM_SUNXI_NEW) || IS_ENABLED(CONFIG_PWM_SUNXI_GROUP)
	if (bl->pwm_info.pwm_dev) {
		if (brightness != 0)
			brightness += 1;
		brightness = (brightness > 255) ? 255 : brightness;
		bl_value = bl->bright_curve_tbl[brightness];


		bl->dimming = bl->dimming ? bl->dimming : 256;

		bl->pwm_info.duty_ns = (bl_value * bl->dimming
			* bl->pwm_info.period_ns / 256 + 128) / 256;
		ret = pwm_config(bl->pwm_info.pwm_dev, bl->pwm_info.duty_ns,
					bl->pwm_info.period_ns);
		if (ret < 0) {
			DRM_ERROR("[SUNXI_BL]%s pwm_config failed\n", __func__);
			return -1;
		}
	}
#endif

	/* need to implement panel set bright here */
	bl->bright = brightness;

	return 0;
}

unsigned int sunxi_backlight_get_bright(unsigned int lcd_id)
{
	struct sunxi_lcd_backlight *bl = sunxi_backlight_get_backlight(lcd_id);

	if (bl)
		return bl->bright;
	else
		return 0;
}

/**
 * @name       :sunxi_backlight_set_backlight_device
 * @brief      :set backligt_device pointer, memer of sunxi_lcd_backlight
 * @param[IN]  :lcd_id: sunxi lcd index
 * @param[IN]  :p_bl_dev:pointer to set
 * @return     :0 if success, -1 else
 */
int sunxi_backlight_set_backlight_device(unsigned int lcd_id,
					 struct backlight_device *p_bl_dev)
{
	int ret = 0;
	struct sunxi_lcd_backlight *bl = sunxi_backlight_get_backlight(lcd_id);

	if (p_bl_dev && bl)
		bl->bl_dev = p_bl_dev;
	else
		ret = -1;

	return ret;
}

/**
 * @name       :sunxi_backlight_get_backlight_device
 * @brief      :get pointer of backlight_device of specified lcd_id
 * @param[IN]  :lcd_id: sunxi lcd index
 * @return     :pointer of backlight_device, NULL if fail to get one
 */
struct backlight_device *
sunxi_backlight_get_backlight_device(unsigned int lcd_id)
{
	struct sunxi_lcd_backlight *bl = sunxi_backlight_get_backlight(lcd_id);

	if (bl)
		return bl->bl_dev;
	return NULL;
}

unsigned int sunxi_backlight_set_bright_dimming(unsigned int lcd_id,
							unsigned int dimming)
{
	struct sunxi_lcd_backlight *bl = NULL;

	bl = sunxi_backlight_get_backlight(lcd_id);
	if (!bl) {
		DRM_ERROR("Null bl pointer!\n");
		return -1;
	}

	bl->dimming = dimming > 256 ? 256 : dimming;
	if (sunxi_backlight_set_bright(lcd_id, bl->bright) < 0) {
		DRM_ERROR("sunxi_backling_set_bright failed\n");
		return -1;
	}

	return 0;
}


int sunxi_backlight_pwm_enable(unsigned int lcd_id)
{
#if IS_ENABLED(CONFIG_AW_PWM) || IS_ENABLED(CONFIG_PWM_SUNXI_NEW) || IS_ENABLED(CONFIG_PWM_SUNXI_GROUP)
	int ret = 0;
	struct sunxi_lcd_backlight *bl = sunxi_backlight_get_backlight(lcd_id);
	struct pwm_device *pwm_dev = bl->pwm_info.pwm_dev;
	struct pwm_state state;

	if (!bl->use_pwm || !bl->pwm_info.pwm_dev) {
		DRM_INFO("Warn: NOT use PWM\n");
		return 0;
	}

	memset(&state, 0, sizeof(state));
	pwm_get_state(pwm_dev, &state);
	state.polarity = bl->pwm_info.polarity;
	state.period = bl->pwm_info.period_ns;
	state.duty_cycle = bl->pwm_info.duty_ns;
	ret = pwm_apply_state(pwm_dev, &state);
	if (ret) {
		DRM_ERROR("[sunxi_bl lcd:%d] pwm_apply_state failed, ret = %d\n", lcd_id, ret);
		return -1;
	}

	sunxi_backlight_set_bright(lcd_id, bl->bright);

	ret = pwm_enable(bl->pwm_info.pwm_dev);
	if (ret < 0) {
		DRM_ERROR("pwm_enable failed\n");
		return -1;
	}
#endif

	return 0;
}

int sunxi_backlight_pwm_disable(unsigned int lcd_id)
{
#if IS_ENABLED(CONFIG_AW_PWM) || IS_ENABLED(CONFIG_PWM_SUNXI_NEW) || IS_ENABLED(CONFIG_PWM_SUNXI_GROUP)
	struct sunxi_lcd_backlight *bl = sunxi_backlight_get_backlight(lcd_id);
	struct pwm_device *pwm_dev = bl->pwm_info.pwm_dev;
	struct pwm_state state;

	if (bl->use_pwm) {
		if (pwm_dev) {
			pwm_disable(pwm_dev);

			/* following is for reset pwm state purpose */
			pwm_config(pwm_dev, pwm_dev->state.duty_cycle - 1,
				   pwm_dev->state.period);
		   memset(&state, 0, sizeof(state));
		   pwm_get_state(pwm_dev, &state);
		   state.polarity = bl->pwm_info.polarity;
		   pwm_apply_state(pwm_dev, &state);
		} else {
			DRM_ERROR("%s NULL pwm_dev hdl\n", __func__);
			return -1;
		}
	}
#endif

	return 0;
}

static int sunxi_backlight_bl_en_enable(struct sunxi_lcd_backlight *bl)
{

	if (!bl->use_bl_en) {
		DRM_INFO("[sunxi_bl lcd:%d] Warn: NOT use bl_en function\n", bl->lcd_id);
		return 0;
	}

	if (!strcmp(bl->bl_en_power, "")
		|| !strcmp(bl->bl_en_power, "none")) {
		DRM_INFO("[sunxi_bl lcd:%d]Warn: do NOT have bl_en_power\n", bl->lcd_id);
		/* return 0; */
	} else if (sunxi_drm_sys_power_enable(bl->dev, bl->bl_en_power) < 0) {
		DRM_ERROR("%s sunxi_drm_sys_power_enable failed\n",
						__func__);
		return -1;
	}

	bl->bl_gpio_hdl = sunxi_drm_sys_gpio_request(&bl->bl_en_gpio);
	if (bl->bl_gpio_hdl < 0) {
		DRM_ERROR("%s sunxi_drm_sys_gpio_request failed\n",
						__func__);
		return -1;
	}


	return 0;
}

static int sunxi_backlight_bl_en_disable(struct sunxi_lcd_backlight *bl)
{
	if (!bl->use_bl_en) {
		DRM_INFO("[sunxi_bl lcd:%d]Warn: NOT use bl_en function\n", bl->lcd_id);
		return 0;
	}

	sunxi_drm_sys_gpio_release(bl->bl_gpio_hdl);

	if (!strcmp(bl->bl_en_power, "")
		|| !strcmp(bl->bl_en_power, "none")) {
		DRM_INFO("[sunxi_bl lcd:%d]Warn: do NOT have bl_en_power\n", bl->lcd_id);
		return 0;
	}

	if (sunxi_drm_sys_power_disable(bl->dev, bl->bl_en_power) < 0) {
		DRM_ERROR("%s sunxi_drm_sys_power_enable failed\n",
						__func__);
		return -1;
	}

	return 0;
}

/*
 * NOTE: This function not only enable bl_en, but also include PWM
 * enable backlight--->set bright
 */
int sunxi_backlight_enable(unsigned int lcd_id)
{
	struct sunxi_lcd_backlight *bl = NULL;
	int ret = -1;

	bl = sunxi_backlight_get_backlight(lcd_id);
	if (!bl) {
		DRM_ERROR("[sunxi_bl lcd:%d] Null bl pointer!\n", lcd_id);
		goto OUT;
	}

	/* if bl_en has been enable, increase that count */
	if (bl->bl_en_count) {
		bl->bl_en_count++;
		DRM_INFO("[sunxi_bl lcd:%d]Warn:bl_en has been enabled, count:%d\n",
			 lcd_id, bl->bl_en_count - 1);
		return 0;
	}

	ret = sunxi_backlight_pwm_enable(lcd_id);
	if (ret) {
		DRM_ERROR("[sunxi_bl lcd:%d] PWM enable fail!\n", lcd_id);
		/* goto OUT; */
	}
	ret = sunxi_backlight_bl_en_enable(bl);
	if (ret) {
		DRM_ERROR("[sunxi_bl lcd:%d] sunxi_backlight_bl_en_enabl failed\n",
						lcd_id);
		goto OUT;
	}

	bl->bl_en_count++;

OUT:
	return ret;
}

int sunxi_backlight_disable(unsigned int lcd_id)
{
	struct sunxi_lcd_backlight *bl = NULL;
	int ret = -1;

	bl = sunxi_backlight_get_backlight(lcd_id);
	if (!bl) {
		DRM_ERROR("[sunxi_bl lcd:%d] Null bl pointer!\n", lcd_id);
		goto OUT;
	}

	if (!bl->bl_en_count) {
		DRM_INFO("[lcd:%d sunxi_bl]Warn:bl_en has NOT been enabled\n", lcd_id);
		return 0;
	}

	bl->bl_en_count--;
	if (bl->bl_en_count) {
		DRM_INFO("[lcd:%d sunxi_bl]Warn:There is another panel use bl_en, "
			" we should NOT close bl_en.   count:%d\n",
						lcd_id, bl->bl_en_count);
		return 0;
	}

	ret = sunxi_backlight_pwm_disable(lcd_id);
	if (ret) {
		DRM_ERROR("[lcd:%d sunxi_bl] PWM disable fail!\n", lcd_id);
		goto OUT;
	}

	ret = sunxi_backlight_bl_en_disable(bl);
	if (ret) {
		DRM_ERROR("[lcd:%d sunxi_bl] sunxi_backlight_bl_en_enabl failed\n",
						lcd_id);
		goto OUT;
	}

OUT:
	return ret;
}

/* get a backlight and init it
 * return: id of backlight
 */
int sunxi_backlight_init(int lcd_id, struct backlight_config *config,
					unsigned int *bl_curve_tbl)
{
	struct sunxi_lcd_backlight *bl;
	struct pwm_info *pwm_info;
	struct pwm_info *pwm_tmp = NULL;
	struct pwm_config *pwm = &config->pwm;
	unsigned long long period_ns, duty_ns;

	if (!bl_curve_tbl || !config) {
		DRM_ERROR("bl_curve_tbl or config NULL hdl\n");
		return -1;
	}

	if (lcd_id >= BL_NUM_MAX) {
		DRM_ERROR("can NOT get a free/idle backlight\n");
		return -1;
	}

	sunxi_backlight_cnt++;
	bl = kzalloc(sizeof(*bl),
				GFP_KERNEL);
	if (!bl) {
		DRM_INFO("[SUNXI-BL] kzalloc for lcd:%d backlinght failed\n",
			lcd_id);
		return -1;
	}

	sunxi_backlight[lcd_id] = bl;
	bl->lcd_id = lcd_id;

	mutex_init(&bl->backlight_lock);
	/* acquire bl_en info */
	bl->use_bl_en = config->use_bl_en;
	memcpy(&bl->bl_en_gpio, &config->bl_en_gpio,
			sizeof(bl->bl_en_gpio));
	memcpy(bl->bl_en_power, config->bl_en_power, 32);

	/* acquire pwm related info */
	pwm_info = &bl->pwm_info;

	memcpy(bl->bright_curve_tbl, bl_curve_tbl, 256 * sizeof(unsigned int));

	bl->bright = config->bright;
	bl->use_pwm = config->use_pwm;

	if (bl->use_pwm) {
		pwm_tmp = sunxi_backlight_get_same_pwm(lcd_id, pwm->lcd_pwm_ch);
		if (pwm_tmp) {
			memcpy(pwm_info, pwm_tmp, sizeof(*pwm_tmp));
			DRM_INFO("[sunxi_bl lcd:%d] Get same pwm, ch:%u\n",
						lcd_id, pwm->lcd_pwm_ch);
			goto out;
		}

		pwm_info->channel = pwm->lcd_pwm_ch;
		pwm_info->polarity = pwm->lcd_pwm_pol;
		if (pwm->lcd_pwm_freq != 0) {
			period_ns = 1000 * 1000 * 1000 / pwm->lcd_pwm_freq;
		} else {
			DRM_INFO("[sunxi_bl lcd:%d] lcd_pwm_freq is ZERO\n",
									lcd_id);
			/* default 1khz */
			period_ns = 1000 * 1000 * 1000 / 1000;
		}

		duty_ns = (bl->bright * period_ns) / 256;
		pwm_info->duty_ns = duty_ns;
		pwm_info->period_ns = period_ns;

#if IS_ENABLED(CONFIG_AW_PWM) || IS_ENABLED(CONFIG_PWM_SUNXI_NEW) || IS_ENABLED(CONFIG_PWM_SUNXI_GROUP)
		pwm_info->pwm_dev = pwm_request(pwm_info->channel, "lcd");
		if ((!pwm_info->pwm_dev) || IS_ERR(pwm_info->pwm_dev)) {
			DRM_ERROR("get pwm device failed\n");
			return -1;
		}
#endif
	}

out:
	return 0;
}

void sunxi_backlight_remove(int lcd_id)
{
	struct sunxi_lcd_backlight *bl = sunxi_backlight_get_backlight(lcd_id);
	struct pwm_info *pwm_info = &bl->pwm_info;

#if IS_ENABLED(CONFIG_AW_PWM) || IS_ENABLED(CONFIG_PWM_SUNXI_NEW) || IS_ENABLED(CONFIG_PWM_SUNXI_GROUP)
	if (bl->use_pwm)
		pwm_free(pwm_info->pwm_dev);
#endif

	kfree(bl);
	sunxi_backlight[lcd_id] = NULL;
}

int sunxi_backlight_drv_init(void)
{
	sunxi_backlight_cnt = 0;
	memset(sunxi_backlight, 0,
		sizeof(struct sunxi_lcd_backlight *) * BL_NUM_MAX);
	return 0;
}

int sunxi_backlight_drv_exit(void)
{
	int i = 0;

	for (i = 0; i < BL_NUM_MAX; i++) {
		if (sunxi_backlight[i]) {
			kfree(sunxi_backlight[i]);
			sunxi_backlight[i] = NULL;
		}
	}

	return 0;
}

/**
 * @name       :sunxi_backlight_get_bl_en_count
 * @brief      :get backlight enable count
 * @param[IN]  :lcd_id: sunxi lcd index
 * @return     :backlight enable count, -1 if fail
 */
int sunxi_backlight_get_bl_en_count(unsigned int lcd_id)
{
	struct sunxi_lcd_backlight *bl = NULL;

	bl = sunxi_backlight_get_backlight(lcd_id);
	if (!bl) {
		DRM_ERROR("Null bl pointer!\n");
		return -1;
	}

	return (int)bl->bl_en_count;
}

#if IS_ENABLED(CONFIG_BACKLIGHT_CLASS_DEVICE)
/* set backlight brightness to level in range [0..max], scaling wrt hw min */
static void sunxi_panel_set_backlight(struct sunxi_lcd_backlight *bl,
				      u32 user_level, u32 user_max)
{
	if (!bl)
		return;

	mutex_lock(&bl->backlight_lock);
	sunxi_backlight_set_bright(bl->lcd_id, user_level);
	mutex_unlock(&bl->backlight_lock);
}

static int sunxi_backlight_device_update_status(struct backlight_device *bd)
{
	struct sunxi_lcd_backlight *bl = bl_get_data(bd);
	u32 enable = 0;

	DRM_DEBUG_KMS("updating sunxi_backlight, brightness=%d/%d\n",
		      bd->props.brightness, bd->props.max_brightness);

	enable = bl->bl_en_count;
	if (bd->props.power != FB_BLANK_POWERDOWN) {
		if (!enable)
			sunxi_backlight_enable(bl->lcd_id);
		sunxi_panel_set_backlight(bl, bd->props.brightness,
					  bd->props.max_brightness);
	} else {
		sunxi_panel_set_backlight(bl, 0,
					  bd->props.max_brightness);
		if (enable)
			sunxi_backlight_disable(bl->lcd_id);
	}

	return 0;
}

static int sunxi_backlight_device_get_brightness(struct backlight_device *bd)
{
	struct sunxi_lcd_backlight *bl = bl_get_data(bd);
	int ret = -1;

	mutex_lock(&bl->backlight_lock);
	ret = bl->bright;
	mutex_unlock(&bl->backlight_lock);

	return ret;
}

static const struct backlight_ops sunxi_backlight_device_ops = {
	.update_status = sunxi_backlight_device_update_status,
	.get_brightness = sunxi_backlight_device_get_brightness,
};

/**
 * @name       :sunxi_backlight_device_register
 * @brief      :register sysfs backlight (Linux backlight system)
 * @param[IN]  :connector:pointer of drm_connector
 * @param[IN]  :lcd_id: index of sunxi_lcd whose backlight's going to be registered
 * @return     :0 if success, -1 else
 */
int sunxi_backlight_device_register(struct device *device,
				unsigned int lcd_id)
{
	struct backlight_properties props;
	int ret = 0;
	char sunxi_bl_name[40] = {0};
	struct backlight_device *p_bl_dev = NULL;
	struct sunxi_lcd_backlight *bl = NULL;

	if (!device) {
		ret = -1;
		DRM_ERROR("Null device pointer!\n");
		goto OUT;
	}

	memset(&props, 0, sizeof(props));
	props.type = BACKLIGHT_RAW;

	bl = sunxi_backlight_get_backlight(lcd_id);

	if (!bl) {
		ret = 0;
		DRM_INFO("[WARN]LCD:%d has NOT backlight\n", lcd_id);
		goto OUT;
	}

	if (!bl->use_pwm) {
		DRM_DEBUG_DRIVER("lcd:%d has NO pwm, need NOT "
			"to register a linux backlight\n", lcd_id);
		ret = 0;
		goto OUT;
	}

	/*
	 * Note: Everything should work even if the backlight device max
	 * presented to the userspace is arbitrarily chosen.
	 */
	props.max_brightness = 255;
	props.brightness = bl->bright;

	props.power = FB_BLANK_UNBLANK;

	snprintf(sunxi_bl_name, 40, "lcd%d_backlight", lcd_id);
	p_bl_dev =
		backlight_device_register(sunxi_bl_name,
					  device, bl,
					  &sunxi_backlight_device_ops, &props);

	if (IS_ERR(p_bl_dev)) {
		DRM_ERROR("Failed to register backlight: %ld\n",
			  PTR_ERR(p_bl_dev));
		p_bl_dev = NULL;
		return -ENODEV;
	}

	bl->dev = device;
	bl->bl_dev = p_bl_dev;

	DRM_DEBUG_KMS("lcd:%d backlight sysfs interface registered\n",
		      lcd_id);

OUT:
	return ret;
}

/**
 * @name       :sunxi_backlight_device_unregister
 * @brief      :Unregister backlight device of lcd: lcd_id
 * @param[IN]  :lcd_id: index of sunxi_lcd
 * @return     :NONE
 */
void sunxi_backlight_device_unregister(unsigned int lcd_id)
{
	struct backlight_device *p_bl_dev = NULL;
	struct sunxi_lcd_backlight *bl = sunxi_backlight_get_backlight(lcd_id);

	p_bl_dev = bl->bl_dev;
	if (p_bl_dev) {
		backlight_device_unregister(p_bl_dev);
		bl->bl_dev = NULL;
	} else
		DRM_WARN("[sunxi_bl lcd:%d] Can not find corresponding backlight device\n", lcd_id);
}
#else
void sunxi_backlight_device_unregister(unsigned int lcd_id)
{
	DRM_WARN("You must select CONFIG_BACKLIGHT_CLASS_DEVICE in menuconfig\n");
}

int sunxi_backlight_device_register(unsigned int lcd_id)
{
	DRM_WARN("You must select CONFIG_BACKLIGHT_CLASS_DEVICE in menuconfig\n");
	return 0;
}
#endif /* CONFIG_BACKLIGHT_CLASS_DEVICE */
