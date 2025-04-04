/*
 * Allwinner SoCs tv driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#include "tv_ac200.h"
#include "tv_ac200_lowlevel.h"
#if defined(CONFIG_EXTCON)
#include <linux/extcon.h>
#endif
#include <sunxi-sid.h>

#include "../disp/de/disp_vdevice.h"
#include "../disp/dev_disp.h"
/* clk */
#define DE_LCD_CLK "lcd0"
#define DE_LCD_CLK_SRC "pll_video0"
static struct clk *tv_clk;
static char modules_name[32] = "tv_ac200";
/* static char key_name[20] = "tv_ac200_para"; */
static enum disp_tv_mode g_tv_mode = DISP_TV_MOD_PAL;

static u32 tv_screen_id;
static u32 tv_used;
static u32 ccir_clk_div;

static struct mutex mlock;
static struct mutex tv_mutex;
static bool tv_suspend_status;
static bool tv_open_status;

static struct disp_device *tv_device;
static struct disp_vdevice_source_ops tv_source_ops;

#if defined(CONFIG_EXTCON)
static const unsigned int ac200_tv_cable[] = {
	EXTCON_DISP_CVBS,
	EXTCON_NONE,
};
#endif

struct ac200_tv_priv tv_priv;
struct disp_video_timings tv_video_timing[] = {
		{
			.vic = 0,
			.tv_mode = DISP_TV_MOD_NTSC,
			.pixel_clk = 54000000,
			.pixel_repeat = 0,
			.x_res = 720,
			.y_res = 480,
			.hor_total_time = 858,
			.hor_back_porch = 57,
			.hor_front_porch = 19,
			.hor_sync_time = 62,
			.ver_total_time = 525,
			.ver_back_porch = 15,
			.ver_front_porch = 4,
			.ver_sync_time = 3,
			.hor_sync_polarity = 0,
			.ver_sync_polarity = 0,
			.b_interlace = 0,
			.vactive_space = 0,
			.trd_mode = 0,
		},
		{
			.vic = 0,
			.tv_mode = DISP_TV_MOD_PAL,
			.pixel_clk = 54000000,
			.pixel_repeat = 0,
			.x_res = 720,
			.y_res = 576,
			.hor_total_time = 864,
			.hor_back_porch = 69,
			.hor_front_porch = 12,
			.hor_sync_time = 63,
			.ver_total_time = 625,
			.ver_back_porch = 19,
			.ver_front_porch = 2,
			.ver_sync_time = 3,
			.hor_sync_polarity = 0,
			.ver_sync_polarity = 0,
			.b_interlace = 0,
			.vactive_space = 0,
			.trd_mode = 0,
		},
};

s32 tv_delay_ms(u32 ms)
{
	u32 timeout = msecs_to_jiffies(ms);

	set_current_state(TASK_UNINTERRUPTIBLE);
	schedule_timeout(timeout);
	return 0;
}

static s32 tv_set_ccir_clk_pin(u32 bon)
{
	return disp_sys_pin_set_state(
	    "ac200", (bon == 1) ? "ccir_clk_active" : "ccir_clk_sleep");
}

#if defined(CONFIG_EXTCON)

static struct task_struct *tv_hpd_task;
static u32 tv_hpd_sourc;

static struct extcon_dev *cvbs_extcon_dev;

void tv_report_hpd_work(void)
{
	switch (tv_hpd_sourc) {

	case DISP_TV_NONE:
		extcon_set_state_sync(cvbs_extcon_dev, EXTCON_DISP_CVBS,
				      STATUE_CLOSE);
		break;

	case DISP_TV_CVBS:
		extcon_set_state_sync(cvbs_extcon_dev, EXTCON_DISP_CVBS,
				      STATUE_OPEN);
		break;

	default:
		extcon_set_state_sync(cvbs_extcon_dev, EXTCON_DISP_CVBS,
				      STATUE_CLOSE);

		break;
	}
}

s32 tv_detect_thread(void *parg)
{
	s32 hpd;
	tv_delay_ms(500);
	tv_set_ccir_clk_pin(1);
	while (1) {
		if (kthread_should_stop())
			break;
		if (!tv_suspend_status) {
			hpd = aw1683_tve_plug_status();
			if (hpd != tv_hpd_sourc) {
				tv_hpd_sourc = hpd;
				tv_report_hpd_work();
			}
		}
		set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(200);
	}
	return 0;
}

s32 tv_detect_enable(void)
{
	tv_hpd_task = kthread_create(tv_detect_thread, (void *)0, "tve detect");
	if (IS_ERR_OR_NULL(tv_hpd_task)) {
		s32 err = 0;

		err = PTR_ERR(tv_hpd_task);
		tv_hpd_task = NULL;
		return err;
	}
	pr_debug("tv_hpd_task is ok!\n");
	wake_up_process(tv_hpd_task);
	return 0;
}

s32 tv_detect_disable(void)
{
	if (tv_hpd_task) {
		kthread_stop(tv_hpd_task);
		tv_hpd_task = NULL;
	}
	return 0;
}

static s32 tv_get_hpd_status(void)
{
	return tv_hpd_sourc;
}

#else
void tv_report_hpd_work(void)
{
	pr_debug("there is null report hpd work,you need support the extcon class!");
}

s32 tv_detect_thread(void *parg)
{
	pr_debug("there is null tv_detect_thread,you need support the extcon class!");
	return -1;
}

s32 tv_detect_enable(void)
{
	pr_debug("there is null tv_detect_enable,you need support the extcon class!");
	return -1;
}

s32 tv_detect_disable(void)
{
	pr_debug("there is null tv_detect_disable,you need support the extcon class!");
	return -1;
}

static s32 tv_get_hpd_status(void)
{
	pr_debug("there is null tv_detect_disable,you need support the extcon class!");
	return -1;
}
#endif

#if defined(CONFIG_ARCH_SUN50IW6)
/**
 * @name       tv_read_sid
 * @brief      read tv out sid from efuse
 * @param[IN]   none
 * @param[OUT]  p_dac_cali:tv_out dac cali
 * @return	return 0 if success,-1 if fail
 */
static s32 tv_read_sid(u16 *p_dac_cali)
{
	s32 ret = 0;
	u8 buf[6];

	if (p_dac_cali == NULL) {
		pr_info("%s's pointer type args are NULL!\n", __func__);
		return -1;
	}
	ret = sunxi_efuse_readn(EFUSE_OEM_NAME, buf, 6);
	if (ret < 0) {
		pr_info("sunxi_efuse_readn failed:%d\n", ret);
		return ret;
	}
	*p_dac_cali = buf[2] + (buf[3] << 8);
	return 0;
}
#endif
#if 0
static s32 tv_power_on(u32 on_off)
{
	if (tv_power_used == 0)
		return 0;
	if (on_off)
		disp_sys_power_enable(tv_power);
	else
		disp_sys_power_disable(tv_power);

	return 0;
}
#endif

static s32 tv_clk_init(void)
{
	/* disp_sys_clk_set_parent(DE_LCD_CLK, DE_LCD_CLK_SRC); */

	return 0;
}

static s32 tv_clk_exit(void)
{

	return 0;
}

static s32 tv_clk_config(u32 mode)
{
	unsigned long pixel_clk, pll_rate, lcd_rate, dclk_rate;
	unsigned long pll_rate_set, lcd_rate_set, dclk_rate_set;
	u32 pixel_repeat, tcon_div, lcd_div;
	struct clk *parent = NULL;

	if (mode == 11) {
		pixel_clk = tv_video_timing[1].pixel_clk;
		pixel_repeat = tv_video_timing[1].pixel_repeat;
	} else {
		pixel_clk = tv_video_timing[0].pixel_clk;
		pixel_repeat = tv_video_timing[0].pixel_repeat;
	}
	lcd_div = 1;
	dclk_rate = pixel_clk * (pixel_repeat + 1);
	tcon_div = ccir_clk_div;
	lcd_rate = dclk_rate * tcon_div;
	pll_rate = lcd_rate * lcd_div;

	parent = clk_get_parent(tv_clk);
	if (parent)
		clk_set_rate(parent, pll_rate);
	pll_rate_set = clk_get_rate(parent);
	lcd_rate_set = pll_rate_set / lcd_div;
	clk_set_rate(tv_clk, lcd_rate_set);
	lcd_rate_set = clk_get_rate(tv_clk);
	dclk_rate_set = lcd_rate_set / tcon_div;
	if (dclk_rate_set != dclk_rate)
		pr_info("pclk=%ld, cur=%ld\n", dclk_rate, dclk_rate_set);

	return 0;
}

static s32 tv_clk_enable(u32 mode)
{
	int ret = 0;

	tv_clk_config(mode);
	if (tv_clk)
		ret = clk_prepare_enable(tv_clk);

	return ret;
}


static s32 tv_clk_disable(void)
{
	if (tv_clk) {
		clk_disable(tv_clk);
		clk_unprepare(tv_clk);
	}

	return 0;
}

static int tv_pin_config(u32 bon)
{
	return disp_sys_pin_set_state("ac200",
		(bon == 1) ? DISP_PIN_STATE_ACTIVE:DISP_PIN_STATE_SLEEP);
}

static s32 tv_open(void)
{
	s32 ret = 0;

	if (mutex_trylock(&tv_mutex)) {
		if (tv_suspend_status == true) {
			mutex_unlock(&tv_mutex);
			return 0;
		}
		tv_set_ccir_clk_pin(1);
		tv_pin_config(1);
		tv_delay_ms(300);
		if (tv_source_ops.tcon_enable) {
			tv_source_ops.tcon_enable(tv_device);
			mutex_lock(&mlock);
			mutex_unlock(&mlock);
		}
		aw1683_tve_set_mode(g_tv_mode);
		ret = aw1683_tve_open();
		if (ret == 0)
			pr_info("[DISP] ac200 tve open finish\n");
		else
			pr_info("[DISP] ac200 tve failed:%d\n", ret);

		tv_open_status = true;
		tv_detect_enable();
		mutex_unlock(&tv_mutex);
	}
	return ret;
}

static s32 tv_close(void)
{
	if (mutex_trylock(&tv_mutex)) {
		if (tv_suspend_status == true) {
			mutex_unlock(&tv_mutex);
			return 0;
		}
		tv_set_ccir_clk_pin(0);
		tv_pin_config(0);
		tv_detect_disable();
		aw1683_tve_close();
		if (tv_source_ops.tcon_disable) {
			tv_source_ops.tcon_disable(tv_device);
			mutex_lock(&mlock);
			mutex_unlock(&mlock);
		}
		/* for hot plug purpose */
		if (__clk_is_enabled(tv_clk) == 0)
			tv_clk_enable(g_tv_mode);
		tv_delay_ms(90);
		if (tv_source_ops.tcon_simple_enable)
			tv_source_ops.tcon_simple_enable(tv_device);

		tv_open_status = false;
		mutex_unlock(&tv_mutex);
	}
	/* for switch resolution purpose */
	tv_delay_ms(450);
	if (tv_open_status == false)
		tv_detect_enable();
	return 0;
}

static s32 tv_set_mode(enum disp_tv_mode tv_mode)
{

	mutex_lock(&mlock);
	g_tv_mode = tv_mode;
	mutex_unlock(&mlock);
	return 0;
}


static s32 tv_get_mode_support(enum disp_tv_mode tv_mode)
{
	if (tv_mode == DISP_TV_MOD_PAL || tv_mode == DISP_TV_MOD_NTSC)
		return 1;

	return 0;
}

static s32 tv_get_video_timing_info(struct disp_video_timings **video_info)
{
	struct disp_video_timings *info;
	int ret = -1;
	int i, list_num;

	info = tv_video_timing;

	list_num = sizeof(tv_video_timing)/sizeof(struct disp_video_timings);
	for (i = 0; i < list_num; i++) {
		if (info->tv_mode == g_tv_mode) {
			*video_info = info;
			ret = 0;
			break;
		}

		info++;
	}
	return ret;
}

/* 0:rgb;  1:yuv */
static s32 tv_get_input_csc(void)
{
#if defined(CONFIG_ARCH_SUN50IW1)
	return 0;
#else
	return 1;
#endif /* endif CONFIG_ARCH_SUN */
}

static s32 tv_get_interface_para(void *para)
{
	struct disp_vdevice_interface_para intf_para;

	intf_para.intf = 0;
	intf_para.sub_intf = 12;
	intf_para.sequence = 0;
	intf_para.clk_phase = 0;
	intf_para.sync_polarity = 0;
	intf_para.ccir_clk_div = ccir_clk_div;
	intf_para.input_csc = tv_get_input_csc();
	if (g_tv_mode == DISP_TV_MOD_NTSC)
		intf_para.fdelay = 2; /* ntsc */
	else
		intf_para.fdelay = 1; /* pal */

	if (para)
		memcpy(para, &intf_para,
		       sizeof(struct disp_vdevice_interface_para));

	return 0;
}

s32 tv_suspend(void)
{
	/* close tv */
	if (mutex_trylock(&tv_mutex)) {
		if (tv_suspend_status == true) {
			mutex_unlock(&tv_mutex);
		} else {
			tv_detect_disable();
			tv_set_ccir_clk_pin(0);
			tv_pin_config(0);
			aw1683_tve_close();
			if (tv_source_ops.tcon_disable) {
				tv_source_ops.tcon_disable(tv_device);
				mutex_lock(&mlock);
				mutex_unlock(&mlock);
			}
			mutex_unlock(&tv_mutex);
			/* for switch resolution purpose */
			tv_delay_ms(450);
		}
	}

	mutex_lock(&mlock);
	if (tv_used && (false == tv_suspend_status)) {
		tv_suspend_status = true;
		if (tv_source_ops.tcon_disable) {
			tv_source_ops.tcon_disable(tv_device);
		}

		while (__clk_is_enabled(tv_clk) > 1) {
			pr_info("%s clk_co1=%d\n", __func__,
			       __clk_is_enabled(tv_clk));
			tv_clk_disable();
			pr_info("%s clk_co2=%d\n", __func__,
			       __clk_is_enabled(tv_clk));
		}
	}

	mutex_unlock(&mlock);
	return 0;
}


s32 tv_resume(void)
{
	int acx00_state = 0;
	u16 dac_cali = 0;
	s32 ret = 0, try_count = 0;

	acx00_state = acx00_enable();
	while (!acx00_state && try_count < 20) {
		++try_count;
		tv_delay_ms(50);
		acx00_state = acx00_enable();
		pr_info("%s: wait for acx00 resume finish:%d\n",
		       __func__, try_count);
	}
	if (try_count == 20) {
		pr_info("exceed the max try count! Try to enable acx00 here!");
		aw1683_enable_chip();
	}

	/* init tve */
	mutex_lock(&mlock);
	if (tv_used && (true == tv_suspend_status)) {
		tv_set_ccir_clk_pin(1);
		if (__clk_is_enabled(tv_clk) == 0)
			tv_clk_enable(g_tv_mode);

#if defined(CONFIG_ARCH_SUN50IW6)
		if (tv_read_sid(&dac_cali) != 0)
			dac_cali = 0;
#endif /* endif CONFIG_ARCH_SUN50IW6 */
		if (tv_source_ops.tcon_simple_enable)
			tv_source_ops.tcon_simple_enable(tv_device);
		else
			pr_info("[DISP] tcon_simple_enable not exist\n");
		aw1683_tve_init(&dac_cali);
		tv_suspend_status = false;
	}
	mutex_unlock(&mlock);

	/* open tv if need */
	if (tv_open_status == false)
		goto OUT;

	if (mutex_trylock(&tv_mutex)) {
		if (tv_suspend_status == true) {
			mutex_unlock(&tv_mutex);
			goto OUT;
		}
		tv_set_ccir_clk_pin(1);
		tv_pin_config(1);
		tv_delay_ms(10);
		if (tv_source_ops.tcon_enable) {
			tv_source_ops.tcon_enable(tv_device);
			mutex_lock(&mlock);
			mutex_unlock(&mlock);
		}
		aw1683_tve_set_mode(g_tv_mode);
		ret = aw1683_tve_open();
		if (ret == 0)
			pr_info("[DISP] ac200 tve open finish\n");
		else
			pr_info("[DISP] ac200 tve failed:%d\n", ret);

		mutex_unlock(&tv_mutex);
	}

OUT:
	tv_detect_enable();
	return ret;
}

static struct disp_device *tv_ac200_register(void)
{
	struct disp_vdevice_init_data init_data;
	struct disp_device *device;

	memset(&init_data, 0, sizeof(struct disp_vdevice_init_data));
	init_data.disp = tv_screen_id;
	memcpy(init_data.name, modules_name, 32);
	init_data.type = DISP_OUTPUT_TYPE_TV;
	init_data.fix_timing = 0;
	init_data.func.enable = tv_open;
	init_data.func.disable = tv_close;
	init_data.func.get_HPD_status = tv_get_hpd_status;
	init_data.func.set_mode = tv_set_mode;
	init_data.func.mode_support = tv_get_mode_support;
	init_data.func.get_video_timing_info = tv_get_video_timing_info;
	init_data.func.get_interface_para = tv_get_interface_para;
	init_data.func.get_input_csc = tv_get_input_csc;
	disp_vdevice_get_source_ops(&tv_source_ops);
	device = disp_vdevice_register(&init_data);

	return device;
}

static int tv_init(struct platform_device *pdev)
{
	int smooth_boot = 0;
	u16 dac_cali = 0;
	unsigned int value, output_type0, output_mode0, output_type1,
	    output_mode1;
	mutex_init(&mlock);
	mutex_init(&tv_mutex);

	/* parse boot para */
	value = disp_boot_para_parse("boot_disp");
	output_type0 = (value >> 8) & 0xff;
	output_mode0 = (value)&0xff;
	output_type1 = (value >> 24) & 0xff;
	output_mode1 = (value >> 16) & 0xff;
	if ((output_type0 == DISP_OUTPUT_TYPE_TV) ||
	    (output_type1 == DISP_OUTPUT_TYPE_TV)) {
		dev_info(&pdev->dev,
			 "[TV]%s:smooth boot, type0 = %d, type1 = %d\n",
			 __func__, output_type0, output_type1);

		if (output_type0 == DISP_OUTPUT_TYPE_TV)
			g_tv_mode = output_mode0;
		else if (output_type1 == DISP_OUTPUT_TYPE_TV)
			g_tv_mode = output_mode1;
		smooth_boot = 1;
	}

/* if support switch class,register it for cvbs hot plugging detect */
#if defined(CONFIG_SWITCH) || defined(CONFIG_ANDROID_SWITCH)
	switch_dev_register(&cvbs_switch_dev);
#elif defined(CONFIG_EXTCON)
	cvbs_extcon_dev =
		devm_extcon_dev_allocate(&pdev->dev, ac200_tv_cable);
	if (IS_ERR_OR_NULL(cvbs_extcon_dev))
		goto err_register;
	cvbs_extcon_dev->name = "cvbs";
	devm_extcon_dev_register(&pdev->dev, cvbs_extcon_dev);
#endif

	tv_suspend_status = 0;
	tv_used = 1;
	tv_clk_init();

	if (__clk_is_enabled(tv_clk) == 0)
		tv_clk_enable(g_tv_mode);

	/* register extern tv module to vdevice */
	tv_device = tv_ac200_register();

	if (IS_ERR_OR_NULL(tv_device)) {
		dev_err(&pdev->dev, "register tv device failed.\n");
		goto err_register;
	}

	if (!smooth_boot) {
#if defined(CONFIG_ARCH_SUN50IW6)
		if (tv_read_sid(&dac_cali) != 0)
			dac_cali = 0;
#endif /* endif CONFIG_ARCH_SUN50IW6 */
		aw1683_tve_init(&dac_cali);
		/* for hot plug purpose */
		if (tv_source_ops.tcon_simple_enable) {
			tv_source_ops.tcon_simple_enable(tv_device);
		} else
			printk("tcon_simple_enable is not exist!\n");
	}

	/* init param */
	tv_detect_enable();

	return 0;

err_register:
	return -1;
}
static int tv_ac200_probe(struct platform_device *pdev)
{
	struct device_node *node;
	struct acx00 *ax;
	struct platform_device *ax_pdev;
	u32 value = 0;

	ax = dev_get_drvdata(pdev->dev.parent);
	if (!ax)
		dev_err(&pdev->dev, "ax is null!\n");
	tv_priv.acx00 = ax;

	node = of_find_compatible_node(NULL, NULL, "allwinner,sunxi-ac200");
	if (!node) {
		pr_debug("%s: of_find_compatible_node fail\n", __func__);
		return -1;
	}
	ax_pdev = of_find_device_by_node(node);
	/* get clk */
	tv_clk =
	    of_clk_get(ax_pdev->dev.of_node, 0); /* modify when mfd is ready. */
	if (IS_ERR_OR_NULL(tv_clk)) {
		dev_err(&pdev->dev, "fail to get clk for tv\n");
		return -1;
	}

	if (of_property_read_u32_array(node, "tv_clk_div", &value, 1)) {
		dev_err(&pdev->dev, "fail to get tv_clk_div for tv\n");
		return -1;
	}

	if (value <= 0) {
		dev_err(&pdev->dev, "Invalid tv_clk_div!\n");
		return -1;
	}

	ccir_clk_div = value;

	tv_init(pdev);
	return 0;
}

int tv_platform_suspend(struct platform_device *dev, pm_message_t state)
{
	s32 ret = -1;
	dev_dbg(&dev->dev, "%s called\n", __func__);
	ret = tv_suspend();
	return ret;
}

int tv_platform_resume(struct platform_device *dev)
{
	s32 ret = -1;
	dev_dbg(&dev->dev, "%s called\n", __func__);
	ret = tv_resume();
	return ret;
}

static void tv_shutdown(struct platform_device *pdev)
{
	#if 0
	struct acx00_priv *acx00 = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec = acx00->codec;

	/* disable lineout */
	snd_soc_update_bits(codec, LINEOUT_CTRL,
			(0x1<<LINEOUTEN), (0<<LINEOUTEN));
	/* disable pa_ctrl */
	gpio_set_value(item.gpio.gpio, 0);
	#endif
}


static int  tv_remove(struct platform_device *pdev) /* delete __devexit */
{
	if (tv_device)
		disp_vdevice_unregister(tv_device);
	tv_device = NULL;
	tv_clk_exit();
	return 0;
}

static struct platform_driver tv_ac200_driver = {
	.driver = {
		.name = "tv",
		.owner = THIS_MODULE,
		/* .of_match_table = sunxi_tv_ac200_match, */
	},
	.probe = tv_ac200_probe,

	.suspend = tv_platform_suspend,
	.resume = tv_platform_resume,

	.remove = tv_remove,
	.shutdown = tv_shutdown,
};

static int tv_ac200_init(void)
{
	int ret = 0;
	int tv_used = 0;

	ret = disp_sys_script_get_item("ac200", "tv_used", &tv_used, 1);
	if (ret != 1 || tv_used != 1) {
		pr_info("%s: ac200 disable, skip init\n", __func__);
		return -1;
	}

	ret = platform_driver_register(&tv_ac200_driver);
	if (ret)
		return -EINVAL;

	return ret;
}
late_initcall(tv_ac200_init);
/* module_platform_driver(tv_ac200_driver); */

MODULE_AUTHOR("zengqi");
MODULE_DESCRIPTION("tv_ac200 driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:tv_ac200");
