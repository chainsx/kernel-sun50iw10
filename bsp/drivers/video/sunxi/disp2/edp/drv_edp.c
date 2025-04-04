/* drv_edp.c
 *
 * Copyright (c) 2007-2017 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
 *
 * edp driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include "drv_edp.h"
#include "../disp/disp_sys_intf.h"
#include <linux/regulator/consumer.h>

static u32 g_edp_num;
struct drv_edp_info_t g_edp_info[EDP_NUM_MAX];
static u32 power_enable_count;

#ifdef CONFIG_AW_AXP
static int edp_power_enable(char *name, bool en)
{
	struct regulator *regu = NULL;
	int ret = -1;

	regu = regulator_get(NULL, name);
	if (IS_ERR(regu)) {
		edp_wrn("Fail to get regulator %s\n", name);
		goto exit;
	}

	if (en == true) {
		/* enalbe regulator */
		ret = regulator_enable(regu);
		if (ret != 0) {
			edp_wrn("Fail to enable regulator %s!\n", name);
			goto exit1;
		}
		edp_dbg("suceess to enable regulator %s!\n", name);
	} else {
		ret = regulator_disable(regu);
		if (ret != 0) {
			edp_wrn("Fail to disable regulator %s!\n", name);
			goto exit1;
		}
		edp_dbg("suceess to disable regulator %s!\n", name);
	}

exit1:
	/* put regulater, when module exit */
	regulator_put(regu);
exit:
	return ret;
}
#else
static int edp_power_enable(char *name, u32 en)
{
	edp_wrn("Not config CONFIG_AW_AXP\n");
	return 0;
}
#endif
/**
 * @name       :edp_clk_enable
 * @brief      :enable or disable edp clk
 * @param[IN]  :sel index of edp
 * @param[IN]  :en 1:enable , 0 disable
 * @return     :0 if success
 */
static s32 edp_clk_enable(u32 sel, bool en)
{
	s32 ret = -1;

	if (g_edp_info[sel].clk) {
		ret = 0;
		if (en)
			ret = clk_prepare_enable(g_edp_info[sel].clk);
		else
			clk_disable_unprepare(g_edp_info[sel].clk);
	}

	return ret;
}

#if defined(CONFIG_SWITCH) || defined(CONFIG_ANDROID_SWITCH)
static struct task_struct *edp_hpd_task;
static u32 edp_hpd[EDP_NUM_MAX];
static struct switch_dev switch_dev[EDP_NUM_MAX];
static char switch_name[20];

s32 edp_report_hpd_work(u32 sel, u32 hpd)
{
	if (edp_hpd[sel] == hpd)
		return -1;

	switch (hpd) {
	case EDP_STATUE_CLOSE:
		switch_set_state(&switch_dev[sel], EDP_STATUE_CLOSE);
		break;

	case EDP_STATUE_OPEN:
		switch_set_state(&switch_dev[sel], EDP_STATUE_OPEN);
		break;

	default:
		switch_set_state(&switch_dev[sel], EDP_STATUE_CLOSE);
		break;
	}
	edp_hpd[sel] = hpd;
	return 0;
}

s32 edp_hpd_detect_thread(void *parg)
{
	s32 hpd[EDP_NUM_MAX];
	struct drv_edp_info_t *p_edp_info = NULL;
	u32 sel = 0;

	if (!parg) {
		edp_wrn("NUll ndl\n");
		return -1;
	}

	p_edp_info = (struct drv_edp_info_t *)parg;
	sel = p_edp_info->dev->id;

	edp_dbg("sel:%d\n", sel);

	while (1) {
		if (kthread_should_stop())
			break;

		set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(200);

		if (!p_edp_info->suspend) {
			hpd[sel] = dp_get_hpd_status(sel);
			if (hpd[sel] != edp_hpd[sel]) {
				edp_dbg("hpd[%d] = %d\n", sel, hpd[sel]);
				edp_report_hpd_work(sel, hpd[sel]);
			}
		}
	}
	return 0;
}

s32 edp_detect_enable(u32 sel)
{
	s32 err = 0;

	if (!edp_hpd_task) {
		edp_hpd_task = kthread_create(edp_hpd_detect_thread,
					      &(g_edp_info[sel]),
					      "edp detect");
		if (IS_ERR(edp_hpd_task)) {
			err = PTR_ERR(edp_hpd_task);
			edp_hpd_task = NULL;
			return err;
		}
		edp_dbg("edp_hpd_task is ok!\n");
		wake_up_process(edp_hpd_task);
	}
	return 0;
}

s32 edp_detect_disable(s32 sel)
{
	if (edp_hpd_task) {
		kthread_stop(edp_hpd_task);
		edp_hpd_task = NULL;
	}
	return 0;
}

#elif defined(CONFIG_EXTCON)
static struct task_struct *edp_hpd_task;
static u32 edp_hpd[EDP_NUM_MAX];
static struct extcon_dev *extcon_edp[EDP_NUM_MAX];
static char switch_name[20];
static const unsigned int edp_cable[] = {
	EXTCON_DISP_EDP,
	EXTCON_NONE,
};

s32 edp_report_hpd_work(u32 sel, u32 hpd)
{
	if (edp_hpd[sel] == hpd)
		return -1;

	switch (hpd) {
	case EDP_STATUE_CLOSE:
		extcon_set_state_sync(extcon_edp[sel], EXTCON_DISP_EDP,
				      EDP_STATUE_CLOSE);
		break;

	case EDP_STATUE_OPEN:
		extcon_set_state_sync(extcon_edp[sel], EXTCON_DISP_EDP,
				      EDP_STATUE_OPEN);
		break;

	default:
		extcon_set_state_sync(extcon_edp[sel], EXTCON_DISP_EDP,
				      EDP_STATUE_CLOSE);
		break;
	}
	edp_hpd[sel] = hpd;
	return 0;
}

s32 edp_hpd_detect_thread(void *parg)
{
	s32 hpd[EDP_NUM_MAX];
	struct drv_edp_info_t *p_edp_info = NULL;
	u32 sel = 0;

	if (!parg) {
		edp_wrn("NUll ndl\n");
		return -1;
	}

	p_edp_info = (struct drv_edp_info_t *)parg;
	sel = p_edp_info->dev->id;

	edp_dbg("sel:%d\n", sel);

	while (1) {
		if (kthread_should_stop())
			break;

		set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(200);

		if (!p_edp_info->suspend) {
			hpd[sel] = dp_get_hpd_status(sel);
			if (hpd[sel] != edp_hpd[sel]) {
				edp_dbg("hpd[%d] = %d\n", sel, hpd[sel]);
				edp_report_hpd_work(sel, hpd[sel]);
			}
		}
	}
	return 0;
}

s32 edp_detect_enable(u32 sel)
{
	s32 err = 0;

	if (!edp_hpd_task) {
		edp_hpd_task = kthread_create(edp_hpd_detect_thread,
					      &(g_edp_info[sel]),
					      "edp detect");
		if (IS_ERR(edp_hpd_task)) {
			err = PTR_ERR(edp_hpd_task);
			edp_hpd_task = NULL;
			return err;
		}
		edp_dbg("edp_hpd_task is ok!\n");
		wake_up_process(edp_hpd_task);
	}
	return 0;
}

s32 edp_detect_disable(s32 sel)
{
	if (edp_hpd_task) {
		kthread_stop(edp_hpd_task);
		edp_hpd_task = NULL;
	}
	return 0;
}
#else
s32 edp_report_hpd_work(u32 sel, u32 hpd)
{
	edp_wrn("You need to config SWITCH or EXTCON!\n");
	return 0;
}
s32 edp_hpd_detect_thread(void *parg)
{
	edp_wrn("You need to config SWITCH or EXTCON!\n");
	return 0;
}
s32 edp_detect_enable(u32 sel)
{
	edp_wrn("You need to config SWITCH or EXTCON!\n");
	return 0;
}
s32 edp_detect_disable(s32 sel)
{
	edp_wrn("You need to config SWITCH or EXTCON!\n");
	return 0;
}
#endif

s32 edp_get_sys_config(u32 disp, struct disp_video_timings *p_info)
{
	s32 ret = -1;
	s32  value = 1;
	char primary_key[20], sub_name[25];

	if (!p_info)
		goto OUT;

	sprintf(primary_key, "edp%d", disp);
	memset(p_info, 0, sizeof(struct disp_video_timings));

	sprintf(sub_name, "edp_io_power");
	g_edp_info[disp].edp_io_power_used = 0;
	ret = disp_sys_script_get_item(
	    primary_key, sub_name, (int *)(g_edp_info[disp].edp_io_power), 2);
	if (ret == 2) {
		g_edp_info[disp].edp_io_power_used = 1;
		mutex_lock(&g_edp_info[disp].mlock);
		ret = edp_power_enable(g_edp_info[disp].edp_io_power, true);
		if (ret) {
			mutex_unlock(&g_edp_info[disp].mlock);
			goto OUT;
		}
		++power_enable_count;
		mutex_unlock(&g_edp_info[disp].mlock);
	}


	ret = disp_sys_script_get_item(primary_key, "edp_x", &value, 1);
	if (ret == 1)
		p_info->x_res = value;

	ret = disp_sys_script_get_item(primary_key, "edp_y", &value, 1);
	if (ret == 1)
		p_info->y_res = value;

	ret = disp_sys_script_get_item(primary_key, "edp_hbp", &value, 1);
	if (ret == 1)
		p_info->hor_back_porch = value;

	ret = disp_sys_script_get_item(primary_key, "edp_ht", &value, 1);
	if (ret == 1)
		p_info->hor_total_time = value;

	ret = disp_sys_script_get_item(primary_key, "edp_hspw", &value, 1);
	if (ret == 1)
		p_info->hor_sync_time = value;

	ret = disp_sys_script_get_item(primary_key, "edp_vt", &value, 1);
	if (ret == 1)
		p_info->ver_total_time = value;

	ret = disp_sys_script_get_item(primary_key, "edp_vspw", &value, 1);
	if (ret == 1)
		p_info->ver_sync_time = value;

	ret = disp_sys_script_get_item(primary_key, "edp_vbp", &value, 1);
	if (ret == 1)
		p_info->ver_back_porch = value;

	ret = disp_sys_script_get_item(primary_key, "edp_rate", &value, 1);
	if (ret == 1) {
		switch (value) {
		case 0:
			g_edp_info[disp].para.edp_rate = BR_1P62G;
			break;
		case 1:
			g_edp_info[disp].para.edp_rate = BR_2P7G;
			break;
		case 2:
			g_edp_info[disp].para.edp_rate = BR_5P4G;
			break;
		default:
			edp_wrn("edp_rate out of range!\n");
			break;
		}
	}

	ret = disp_sys_script_get_item(primary_key, "edp_lane", &value, 1);
	if (ret == 1)
		g_edp_info[disp].para.edp_lane = value;

	ret = disp_sys_script_get_item(primary_key, "edp_training_func", &value,
				       1);
	if (ret == 1)
		g_edp_info[disp].para.edp_training_func = value;

	ret = disp_sys_script_get_item(primary_key, "edp_sramble_seed", &value,
				       1);
	if (ret == 1)
		g_edp_info[disp].para.edp_sramble_seed = value;

	ret =
	    disp_sys_script_get_item(primary_key, "edp_colordepth", &value, 1);
	if (ret == 1)
		g_edp_info[disp].para.edp_colordepth = value;

	ret = disp_sys_script_get_item(primary_key, "edp_fps", &value, 1);
	if (ret == 1)
		g_edp_info[disp].para.edp_fps = value;

	p_info->pixel_clk = p_info->hor_total_time * p_info->ver_total_time *
			    g_edp_info[disp].para.edp_fps;

OUT:
	return 0;
}
/**
 * @name       edp_disable
 * @brief      disable edp module
 * @param[IN]  sel:index of edp
 * @param[OUT] none
 * @return     0 if success
 */
s32 edp_disable(u32 sel)
{
	s32 ret = 0;

	edp_here;
	mutex_lock(&g_edp_info[sel].mlock);
	if (g_edp_info[sel].enable) {
		dp_disable(sel);
		g_edp_info[sel].enable = 0;
	}
	mutex_unlock(&g_edp_info[sel].mlock);
	return ret;
}

/**
 * @name       edp_enable
 * @brief      edp enable
 * @param[IN]  sel index of edp
 * @return     0 if success
 */
s32 edp_enable(u32 sel)
{
	s32 ret = 0;

	edp_here;
	if (!g_edp_info[sel].enable) {
		ret = dp_enable(sel, &g_edp_info[sel].para,
				&g_edp_info[sel].timings);
		if (ret)
			goto OUT;
		mutex_lock(&g_edp_info[sel].mlock);
		g_edp_info[sel].enable = 1;
		mutex_unlock(&g_edp_info[sel].mlock);
	}

OUT:
	return ret;
}

s32 edp_resume(u32 sel)
{
	s32 ret = -1;

	edp_here;
	mutex_lock(&g_edp_info[sel].mlock);
	if (g_edp_info[sel].suspend) {
		edp_clk_enable(sel, true);
		g_edp_info[sel].suspend = true;
		ret = dp_enable(sel, &g_edp_info[sel].para,
				&g_edp_info[sel].timings);
		edp_detect_enable(sel);
		if (g_edp_info[sel].edp_io_power_used && !power_enable_count) {
			edp_power_enable(g_edp_info[sel].edp_io_power, true);
			++power_enable_count;
		}
	}
	mutex_unlock(&g_edp_info[sel].mlock);
	return ret;
}

s32 edp_suspend(u32 sel)
{
	s32 ret = -1;

	edp_here;
	mutex_lock(&g_edp_info[sel].mlock);
	if (!g_edp_info[sel].suspend) {
		g_edp_info[sel].suspend = true;
		edp_detect_disable(sel);
		ret = edp_clk_enable(sel, false);
		if (g_edp_info[sel].edp_io_power_used && power_enable_count) {
			ret = edp_power_enable(g_edp_info[sel].edp_io_power,
					       false);
			--power_enable_count;
		}
	}
	mutex_unlock(&g_edp_info[sel].mlock);
	return ret;
}

/**
 * @name       edp_get_video_timing_info
 * @brief      get timing info
 * @param[IN]  sel:index of edp module
 * @param[OUT] video_info:timing info
 * @return     0 if success
 */
static s32 edp_get_video_timing_info(u32 sel,
				      struct disp_video_timings **video_info)
{
	s32 ret = 0;

	edp_here;
	*video_info = &g_edp_info[sel].timings;
	return ret;
}


static s32 edp_init(struct platform_device *pdev)
{
	s32 ret = -1;
	u32 value, output_type0, output_mode0, sel = pdev->id, output_type1,
					       output_mode1;

	edp_dbg("start edp init\n");

	mutex_init(&g_edp_info[sel].mlock);

	pdev->dev.id = pdev->id;

	g_edp_info[sel].dev = &pdev->dev;

	ret = edp_get_sys_config(sel, &g_edp_info[sel].timings);
	if (ret != 0)
		goto OUT;

	dp_set_reg_base(sel, g_edp_info[sel].base_addr);

	value = disp_boot_para_parse("boot_disp");
	output_type0 = (value >> 8) & 0xff;
	output_mode0 = (value)&0xff;

	output_type1 = (value >> 24) & 0xff;
	output_mode1 = (value >> 16) & 0xff;
	if ((output_type0 == DISP_OUTPUT_TYPE_EDP) ||
	    (output_type1 == DISP_OUTPUT_TYPE_EDP)) {
		g_edp_info[sel].enable = 1;
	} else {
		dp_hpd_enable(pdev->id, g_edp_info[pdev->id].para.edp_lane,
			      g_edp_info[pdev->id].para.edp_rate);
	}
#if defined(CONFIG_SWITCH) || defined(CONFIG_ANDROID_SWITCH)
	snprintf(switch_name, sizeof(switch_name), "edp%d", sel);
	switch_dev[sel].name = switch_name;
	switch_dev_register(&switch_dev[sel]);
#elif defined(CONFIG_EXTCON)
	snprintf(switch_name, sizeof(switch_name), "edp%d", sel);
	extcon_edp[sel] = devm_extcon_dev_allocate(&pdev->dev, edp_cable);
	if (IS_ERR_OR_NULL(extcon_edp[sel])) {
		edp_wrn("devm_extcon_dev_allocate fail:%d", sel);
		goto OUT;
	}
	extcon_edp[sel]->name = switch_name;
	ret = devm_extcon_dev_register(&pdev->dev, extcon_edp[sel]);
#endif
OUT:
	edp_dbg("end of edp init:%d\n", ret);
	return ret;
}

s32 edp_get_start_delay(u32 sel)
{
	s32 ret = -1;

	ret = dp_get_start_dly(sel);
	return ret;
}

void edp_show_builtin_patten(u32 sel, u32 patten)
{
	dp_show_builtin_patten(sel, patten);
}

unsigned int edp_get_cur_line(u32 sel)
{
	u32 ret = 0;

	ret = dp_get_cur_line(sel);
	return ret;
}

s32 edp_irq_enable(u32 sel, u32 irq_id, u32 en)
{
	s32 ret = 0;

	edp_here;
	dp_int_enable(sel, LINE0, (bool)en);
	return ret;
}

s32 edp_irq_query(u32 sel)
{
	s32 ret = -1;

	ret = dp_int_query(sel, LINE0);
	if (ret == 1)
		dp_int_clear(sel, LINE0);
	return ret;
}

static s32 edp_probe(struct platform_device *pdev)
{
	struct disp_tv_func edp_func;
	s32 ret = -1;

	dev_warn(&pdev->dev, "Welcome to edp_probe %d\n", g_edp_num);

	if (!g_edp_num)
		memset(&g_edp_info, 0,
		       sizeof(struct drv_edp_info_t) * EDP_NUM_MAX);

	if (g_edp_num > EDP_NUM_MAX - 1) {
		dev_err(&pdev->dev,
			"g_edp_num(%d) is greater then EDP_NUM_MAX-1(%d)\n",
			g_edp_num, EDP_NUM_MAX - 1);
		goto OUT;
	}

	pdev->id = of_alias_get_id(pdev->dev.of_node, "edp");
	if (pdev->id < 0) {
		dev_err(&pdev->dev, "failed to get alias id\n");
		goto OUT;
	}

	g_edp_info[g_edp_num].base_addr =
	    (uintptr_t __force)of_iomap(pdev->dev.of_node, 0);
	if (!g_edp_info[g_edp_num].base_addr) {
		dev_err(&pdev->dev, "fail to get addr for edp%d!\n", pdev->id);
		goto ERR_IOMAP;
	}

	g_edp_info[pdev->id].clk = of_clk_get(pdev->dev.of_node, 0);
	if (IS_ERR_OR_NULL(g_edp_info[pdev->id].clk)) {
		dev_err(&pdev->dev, "fail to get clk for edp%d!\n", pdev->id);
		goto ERR_IOMAP;
	}

	ret = edp_clk_enable(pdev->id, true);
	if (ret) {
		dev_err(&pdev->dev, "edp%d edp_clk_enable fail!!\n", pdev->id);
		goto ERR_IOMAP;
	}

	ret = edp_init(pdev);
	if (ret) {
		dev_err(&pdev->dev, "edp_init for edp%d fail!\n", pdev->id);
		goto ERR_IOMAP;
	}


	ret = edp_detect_enable(pdev->id);
	if (ret) {
		dev_err(&pdev->dev, "edp_detect_enable fail!\n");
		goto ERR_CLK;
	}

	if (!g_edp_num) {
		memset(&edp_func, 0, sizeof(struct disp_tv_func));
		edp_func.tv_enable = edp_enable;
		edp_func.tv_disable = edp_disable;
		edp_func.tv_resume = edp_resume;
		edp_func.tv_suspend = edp_suspend;
		edp_func.tv_get_video_timing_info = edp_get_video_timing_info;
		edp_func.tv_irq_enable = edp_irq_enable;
		edp_func.tv_irq_query = edp_irq_query;
		edp_func.tv_get_startdelay = edp_get_start_delay;
		edp_func.tv_get_cur_line = edp_get_cur_line;
		edp_func.tv_show_builtin_patten = edp_show_builtin_patten;
		ret = disp_set_edp_func(&edp_func);
		if (ret) {
			dev_err(&pdev->dev, "disp_set_edp_func edp%d fail!\n",
				pdev->id);
			goto ERR_HPD_DETECT;
		}
	} else
		ret = 0;
	++g_edp_num;
	if (ret == 0)
		goto OUT;

ERR_HPD_DETECT:
	edp_detect_enable(pdev->id);
ERR_CLK:
	edp_clk_enable(pdev->id, false);
ERR_IOMAP:
	if (g_edp_info[g_edp_num].base_addr)
		iounmap((char __iomem *)g_edp_info[g_edp_num].base_addr);
OUT:
	return ret;
}

s32 edp_remove(struct platform_device *pdev)
{
	s32 ret = 0;
	u32 i = 0;

	for (i = 0; i < g_edp_num; ++i) {
		edp_detect_disable(i);
		if (g_edp_info[i].edp_io_power_used && power_enable_count) {
			edp_power_enable(g_edp_info[i].edp_io_power, false);
			--power_enable_count;
		}
		edp_disable(i);
		edp_clk_enable(i, false);
	}

	return ret;
}

static const struct of_device_id sunxi_edp_match[] = {
	{
		.compatible = "allwinner,sunxi-edp0",
	},
	{
		.compatible = "allwinner,sunxi-edp1",
	},
	{},
};

static struct platform_driver edp_driver = {
	.probe = edp_probe,
	.remove = edp_remove,
	.driver = {
		.name = "edp",
		.owner = THIS_MODULE,
		.of_match_table = sunxi_edp_match,
	},
};

s32 __init edp_module_init(void)
{
	s32 ret = 0;

	ret = platform_driver_register(&edp_driver);

	return ret;
}

static void __exit edp_module_exit(void)
{
	platform_driver_unregister(&edp_driver);
}


late_initcall(edp_module_init);
module_exit(edp_module_exit);

MODULE_AUTHOR("zhengxiaobin");
MODULE_DESCRIPTION("edp driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:edp");
/* End of File */
