/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2017 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#include "disp_vdpo.h"

#if defined(SUPPORT_VDPO)

struct disp_vdpo_private_data {
	u32 enabled;
	bool suspended;
	u32 vdpo_index;
	enum disp_tv_mode mode;
	struct disp_tv_func vdpo_func;
	struct disp_video_timings *video_info;
	struct disp_clk_info lcd_clk;
	struct clk *clk;
	struct clk *clk_parent;
	u32 irq_no;
	u32 frame_per_sec;
	u32 usec_per_line;
	u32 judge_line;
};

#define DISP2_VDPO_USE_TCON_S_IRQ
/* global static variable */
static u32 g_vdpo_used;
static struct disp_device *g_pvdpo_devices;
static struct disp_vdpo_private_data *g_pvdpo_private;
static spinlock_t g_vdpo_data_lock;

/**
 * @name       disp_vdpo_get_priv
 * @brief      get disp_vdpo_private_data of disp_device
 * @param[IN]  p_vdpo: disp_device var
 * @param[OUT] none
 * @return     0 if success,otherwise fail
 */
static struct disp_vdpo_private_data *
disp_vdpo_get_priv(struct disp_device *p_vdpo)
{
	if (p_vdpo == NULL) {
		DE_WRN("NULL hdl!\n");
		return NULL;
	}

	return (struct disp_vdpo_private_data *)p_vdpo->priv_data;
}

s32 disp_vdpo_get_fps(struct disp_device *p_vdpo)
{
	struct disp_vdpo_private_data *p_vdpop = disp_vdpo_get_priv(p_vdpo);

	if ((p_vdpo == NULL) || (p_vdpop == NULL)) {
		DE_WRN("p_vdpo set func null  hdl!\n");
		return 0;
	}

	return p_vdpop->frame_per_sec;
}

s32 disp_vdpo_resume(struct disp_device *p_vdpo)
{
	struct disp_vdpo_private_data *p_vdpop = disp_vdpo_get_priv(p_vdpo);
	s32 ret = 0;

	if ((p_vdpo == NULL) || (p_vdpop == NULL)) {
		DE_WRN("disp_vdpo_resume null hdl!\n");
		return 0;
	}
	if (p_vdpop->suspended == true) {
		p_vdpop->suspended = false;
		if (p_vdpop->vdpo_func.tv_resume != NULL)
			ret = p_vdpop->vdpo_func.tv_resume(p_vdpop->vdpo_index);
	}

	return ret;
}

s32 disp_vdpo_suspend(struct disp_device *p_vdpo)
{
	struct disp_vdpo_private_data *p_vdpop = disp_vdpo_get_priv(p_vdpo);
	s32 ret = 0;

	if ((p_vdpo == NULL) || (p_vdpop == NULL)) {
		DE_WRN("disp_vdpo_resume null hdl!\n");
		return 0;
	}
	if (p_vdpop->suspended == false) {
		p_vdpop->suspended = true;
		if (p_vdpop->vdpo_func.tv_suspend != NULL)
			ret =
			    p_vdpop->vdpo_func.tv_suspend(p_vdpop->vdpo_index);
	}
	return ret;
}

s32 disp_vdpo_get_input_csc(struct disp_device *p_vdpo)
{
	struct disp_vdpo_private_data *p_vdpop = disp_vdpo_get_priv(p_vdpo);

	if ((p_vdpo == NULL) || (p_vdpop == NULL)) {
		DE_WRN("disp set func null  hdl!\n");
		return DIS_FAIL;
	}

	if (p_vdpop->vdpo_func.tv_get_input_csc == NULL)
		return DIS_FAIL;

	return p_vdpop->vdpo_func.tv_get_input_csc(p_vdpop->vdpo_index);
}

s32 disp_vdpo_check_support_mode(struct disp_device *p_vdpo,
				 enum disp_tv_mode tv_mode)
{

	struct disp_vdpo_private_data *p_vdpop = disp_vdpo_get_priv(p_vdpo);

	if ((p_vdpo == NULL) || (p_vdpop == NULL)) {
		DE_WRN("vdpo set func null  hdl!\n");
		return DIS_FAIL;
	}

	if (!p_vdpop->vdpo_func.tv_mode_support)
		return 0;

	return p_vdpop->vdpo_func.tv_mode_support(p_vdpop->vdpo_index, tv_mode);
}

/**
 * disp_vdpo_check_if_enabled - check tv if be enabled status
 *
 * this function only be used by bsp_disp_sync_with_hw to check
 * the device enabled status when driver init
 */
static s32 disp_vdpo_check_if_enabled(struct disp_device *p_vdpo)
{
	struct disp_vdpo_private_data *p_vdpop = disp_vdpo_get_priv(p_vdpo);
	int ret = 1;

	if ((p_vdpo == NULL) || (p_vdpop == NULL)) {
		DE_WRN("vdpo set func null  hdl!\n");
		return DIS_FAIL;
	}

#if !defined(CONFIG_COMMON_CLK_ENABLE_SYNCBOOT)
	if (p_vdpop->clk && (__clk_is_enabled(p_vdpop->clk) == 0))
		ret = 0;
#endif

	return ret;
}

static s32 vdpo_calc_judge_line(struct disp_device *p_vdpo)
{
	struct disp_vdpo_private_data *p_vdpop = disp_vdpo_get_priv(p_vdpo);

	int start_delay, usec_start_delay;
	int usec_judge_point;
	int pixel_clk;
	u64 temp = 0;

	if (!p_vdpop || !p_vdpo) {
		DE_WRN("vdpo init null hdl!\n");
		return DIS_FAIL;
	}

	pixel_clk = p_vdpop->video_info->pixel_clk;

	/*
	 * usec_per_line = 1 / fps / vt * 1000000
	 *               = 1 / (pixel_clk / vt / ht) / vt * 1000000
	 *               = ht / pixel_clk * 1000000
	 */
	p_vdpop->frame_per_sec = pixel_clk
	    / p_vdpop->video_info->hor_total_time
	    / p_vdpop->video_info->ver_total_time
	    * (p_vdpop->video_info->b_interlace + 1)
	    / (p_vdpop->video_info->trd_mode + 1);

	temp = (u64)p_vdpop->video_info->hor_total_time * 1000000ull;
	do_div(temp, pixel_clk);
	p_vdpop->usec_per_line = temp;

	start_delay = disp_al_device_get_start_delay(p_vdpo->hwdev_index);
	usec_start_delay = start_delay * p_vdpop->usec_per_line;

	if (usec_start_delay <= 200)
		usec_judge_point = usec_start_delay * 3 / 7;
	else if (usec_start_delay <= 400)
		usec_judge_point = usec_start_delay / 2;
	else
		usec_judge_point = 200;
	p_vdpop->judge_line = usec_judge_point / p_vdpop->usec_per_line;

	return 0;
}

#if defined(__LINUX_PLAT__)
static s32 disp_vdpo_event_proc(int irq, void *parg)
#else
static s32 disp_vdpo_event_proc(void *parg)
#endif
{
	struct disp_device *p_vdpo = (struct disp_device *)parg;
	struct disp_manager *mgr = NULL;
	u32 hwdev_index;
	struct disp_vdpo_private_data *p_vdpop = disp_vdpo_get_priv(p_vdpo);

	if (p_vdpo == NULL || p_vdpop == NULL)
		return DISP_IRQ_RETURN;

	hwdev_index = p_vdpo->hwdev_index;


#if defined(DISP2_VDPO_USE_TCON_S_IRQ)
	if (disp_al_device_query_irq(hwdev_index)) {
		int cur_line = disp_al_device_get_cur_line(hwdev_index);
		int start_delay = disp_al_device_get_start_delay(hwdev_index);
#else
		if (p_vdpop->vdpo_func.tv_irq_query(p_vdpop->vdpo_index)) {
			int cur_line = p_vdpop->vdpo_func.tv_get_cur_line(
			    p_vdpop->vdpo_index);
			int start_delay =
			    disp_al_device_get_start_delay(p_vdpop->vdpo_index);
#endif /* endif  DISP2_VDPO_USE_TCON_S_IRQ */

		mgr = p_vdpo->manager;
		if (mgr == NULL)
			return DISP_IRQ_RETURN;

		if (cur_line <= (start_delay - 4))
			sync_event_proc(mgr->disp, false);
		else
			sync_event_proc(mgr->disp, true);
	}

	return DISP_IRQ_RETURN;
}


s32 disp_vdpo_is_enabled(struct disp_device *p_vdpo)
{
	struct disp_vdpo_private_data *p_vdpop = disp_vdpo_get_priv(p_vdpo);

	if ((p_vdpo == NULL) || (p_vdpop == NULL)) {
		DE_WRN("vdpo set func null  hdl!\n");
		return DIS_FAIL;
	}

	return p_vdpop->enabled;
}

/**
 * @name       :vdpo_clk_config
 * @brief      :set vdpo clk's rate
 * @param[IN]  :p_vdpo:disp_device
 * @param[OUT] :none
 * @return     :0 if success else fail
 */
static s32 vdpo_clk_config(struct disp_device *p_vdpo)
{
	struct disp_vdpo_private_data *p_vdpop = disp_vdpo_get_priv(p_vdpo);
	s32 ret = 0;
	unsigned long rate = 0, rate_set = 0;

	if (!p_vdpo || !p_vdpop) {
		DE_WRN("vdpo clk config null hdl!\n");
		return DIS_FAIL;
	}

	if (p_vdpop->clk_parent)
		clk_set_rate(p_vdpop->clk_parent, 297000000);

	rate = p_vdpo->timings.pixel_clk * 4; /* tcon_dvi==4 */
	if (p_vdpop->clk)
		ret = clk_set_rate(p_vdpop->clk, rate);

	rate_set = clk_get_rate(p_vdpop->clk);
	if (rate_set != rate) {
		if (p_vdpop->clk_parent)
			clk_set_rate(p_vdpop->clk_parent, rate);
		if (p_vdpop->clk)
			ret = clk_set_rate(p_vdpop->clk, rate);
	}
	return ret;
}

static s32 vdpo_clk_disable(struct disp_device *p_vdpo)
{
	struct disp_vdpo_private_data *p_vdpop = disp_vdpo_get_priv(p_vdpo);

	if (!p_vdpo || !p_vdpop) {
		DE_WRN("vdpo init null hdl!\n");
		return DIS_FAIL;
	}

	clk_disable(p_vdpop->clk);

	return 0;
}

/**
 * @name       :vdpo_clk_enable
 * @brief      :config vdpo clk then enable
 * @param[IN]  :p_vdpo:disp_device that contain vdpo's clk
 * @param[OUT] :none
 * @return     :0 if success else fail
 */
static s32 vdpo_clk_enable(struct disp_device *p_vdpo)
{
	struct disp_vdpo_private_data *p_vdpop = disp_vdpo_get_priv(p_vdpo);
	int ret = 0;

	if (!p_vdpo || !p_vdpop) {
		DE_WRN("vdpo clk init null hdl!\n");
		return DIS_FAIL;
	}
	vdpo_clk_config(p_vdpo);
	if (p_vdpop->clk) {
		ret = clk_prepare_enable(p_vdpop->clk);
		if (ret != 0)
			DE_WRN("fail enable vdpo's clock!\n");
	}

	return ret;
}


/**
 * @name       :disp_vdpo_set_func
 * @brief      :set vdpo lowlevel function
 * @param[IN]  :p_vdpo:disp_device
 * @param[OUT] :func:lowlevel function
 * @return     :0 if success else fail
 */
s32 disp_vdpo_set_func(struct disp_device *p_vdpo,
		       struct disp_tv_func *func)
{
	struct disp_vdpo_private_data *p_vdpop = disp_vdpo_get_priv(p_vdpo);

	if ((p_vdpo == NULL) || (p_vdpop == NULL) || (func == NULL)) {
		DE_WRN("vdpo set func null  hdl!\n");
		DE_WRN("%s,point  p_vdpo = %p, point  p_vdpop = %p\n", __func__,
		       p_vdpo, p_vdpop);
		return DIS_FAIL;
	}
	p_vdpop->vdpo_func.tv_enable = func->tv_enable;
	p_vdpop->vdpo_func.tv_disable = func->tv_disable;
	p_vdpop->vdpo_func.tv_suspend = func->tv_suspend;
	p_vdpop->vdpo_func.tv_resume = func->tv_resume;
	p_vdpop->vdpo_func.tv_get_mode = func->tv_get_mode;
	p_vdpop->vdpo_func.tv_set_mode = func->tv_set_mode;
	p_vdpop->vdpo_func.tv_get_input_csc = func->tv_get_input_csc;
	p_vdpop->vdpo_func.tv_get_video_timing_info =
	    func->tv_get_video_timing_info;
	p_vdpop->vdpo_func.tv_mode_support = func->tv_mode_support;
	p_vdpop->vdpo_func.tv_irq_enable = func->tv_irq_enable;
	p_vdpop->vdpo_func.tv_irq_query = func->tv_irq_query;
	p_vdpop->vdpo_func.tv_get_cur_line = func->tv_get_cur_line;
	p_vdpop->vdpo_func.vdpo_set_config = func->vdpo_set_config;

	return 0;
}


static s32 vdpo_clk_init(struct disp_device *p_vdpo)
{
	struct disp_vdpo_private_data *p_vdpop = disp_vdpo_get_priv(p_vdpo);

	if (!p_vdpo || !p_vdpop) {
		DE_WRN("null hdl!\n");
		return DIS_FAIL;
	}

	if (p_vdpop->clk)
		p_vdpop->clk_parent = clk_get_parent(p_vdpop->clk);

	if (!p_vdpop->clk_parent)
		DE_WRN("[VDPO] get tcon clk's parent fail\n");


	return 0;
}

/**
 * @name       :disp_vdpo_init
 * @brief      :get clk if needed
 * @param[IN]  :p_vdpo:disp_device
 * @param[OUT] :none
 * @return     :0 if success else fail
 */
static s32 disp_vdpo_init(struct disp_device *p_vdpo)
{
	struct disp_vdpo_private_data *p_vdpop = disp_vdpo_get_priv(p_vdpo);

	if (!p_vdpo || !p_vdpop) {
		DE_WRN("vdpo init null hdl!\n");
		return DIS_FAIL;
	}

	vdpo_clk_init(p_vdpo);
	return 0;
}

static s32 vdpo_clk_exit(struct disp_device *p_vdpo)
{
	struct disp_vdpo_private_data *p_vdpop = disp_vdpo_get_priv(p_vdpo);

	if (!p_vdpo || !p_vdpop) {
		DE_WRN("vdpo exit null hdl!\n");
		return DIS_FAIL;
	}

	return 0;
}

s32 disp_vdpo_disable(struct disp_device *p_vdpo)
{
	struct disp_vdpo_private_data *p_vdpop = disp_vdpo_get_priv(p_vdpo);
	unsigned long flags;
	struct disp_manager *mgr = NULL;

	if ((p_vdpo == NULL) || (p_vdpop == NULL)) {
		DE_WRN("vdpo set func null  hdl!\n");
		return DIS_FAIL;
	}

	mgr = p_vdpo->manager;
	if (!mgr) {
		DE_WRN("vdpo%d's mgr is NULL\n", p_vdpo->disp);
		return DIS_FAIL;
	}

	if (p_vdpop->enabled == 0) {
		DE_WRN("vdpo%d is already closed\n", p_vdpo->disp);
		return DIS_FAIL;
	}

	if (p_vdpop->vdpo_func.tv_disable == NULL) {
		DE_WRN("tv_func.tv_disable is NULL\n");
		return -1;
	}

	spin_lock_irqsave(&g_vdpo_data_lock, flags);
	p_vdpop->enabled = 0;
	spin_unlock_irqrestore(&g_vdpo_data_lock, flags);

	p_vdpop->vdpo_func.tv_disable(p_vdpop->vdpo_index);

	disp_al_vdpo_disable(p_vdpo->hwdev_index);
	if (mgr->disable)
		mgr->disable(mgr);

	vdpo_clk_disable(p_vdpo);

	p_vdpop->video_info = NULL;

	disp_sys_disable_irq(p_vdpop->irq_no);
	disp_sys_unregister_irq(p_vdpop->irq_no, disp_vdpo_event_proc,
				(void *)p_vdpo);
	disp_delay_ms(1000);
	return 0;
}

static s32 disp_vdpo_exit(struct disp_device *p_vdpo)
{
	struct disp_vdpo_private_data *p_vdpop = disp_vdpo_get_priv(p_vdpo);

	if (!p_vdpo || !p_vdpop) {
		DE_WRN("vdpo init null hdl!\n");
		return DIS_FAIL;
	}

	disp_vdpo_disable(p_vdpo);
	vdpo_clk_exit(p_vdpo);
	kfree(p_vdpo);
	kfree(p_vdpop);
	return 0;
}

s32 disp_vdpo_get_mode(struct disp_device *p_vdpo)
{
	struct disp_vdpo_private_data *p_vdpop = disp_vdpo_get_priv(p_vdpo);
	enum disp_tv_mode tv_mode;

	if ((p_vdpo == NULL) || (p_vdpop == NULL)) {
		DE_WRN("vdpo set mode null  hdl!\n");
		return DIS_FAIL;
	}

	if (p_vdpop->vdpo_func.tv_get_mode == NULL) {
		DE_WRN("tv_get_mode is null!\n");
		return DIS_FAIL;
	}

	tv_mode = p_vdpop->vdpo_func.tv_get_mode(p_vdpop->vdpo_index);
	if (tv_mode != p_vdpop->mode)
		p_vdpop->mode = tv_mode;

	return p_vdpop->mode;
}

s32 disp_vdpo_set_mode(struct disp_device *p_vdpo, enum disp_tv_mode tv_mode)
{
	s32 ret = 0;
	struct disp_vdpo_private_data *p_vdpop = disp_vdpo_get_priv(p_vdpo);

	if ((p_vdpo == NULL) || (p_vdpop == NULL)) {
		DE_WRN("vdpo set mode null  hdl!\n");
		return DIS_FAIL;
	}

	if (p_vdpop->vdpo_func.tv_set_mode == NULL) {
		DE_WRN("tv_set_mode is null!\n");
		return DIS_FAIL;
	}

	ret = p_vdpop->vdpo_func.tv_set_mode(p_vdpop->vdpo_index, tv_mode);
	if (ret == 0)
		p_vdpop->mode = tv_mode;

	return ret;
}

/**
 * @name       :disp_vdpo_enable
 * @brief      :enable vdpo,tcon,register irq,set manager
 * @param[IN]  :p_vdpo disp_device
 * @return     :0 if success
 */
s32 disp_vdpo_enable(struct disp_device *p_vdpo)
{
	unsigned long flags;
	struct disp_vdpo_private_data *p_vdpop = disp_vdpo_get_priv(p_vdpo);
	struct disp_manager *mgr = NULL;
	int ret;

	if ((p_vdpo == NULL) || (p_vdpop == NULL)) {
		DE_WRN("vdpo set func null  hdl!\n");
		return DIS_FAIL;
	}
	DE_INF("%s, disp%d\n", __func__, p_vdpo->disp);

	if (p_vdpop->enabled) {
		DE_WRN("vdpo%d is already enable\n", p_vdpo->disp);
		return DIS_FAIL;
	}

	mgr = p_vdpo->manager;
	if (!mgr) {
		DE_WRN("vdpo%d's mgr is NULL\n", p_vdpo->disp);
		return DIS_FAIL;
	}

	if (p_vdpop->vdpo_func.tv_get_video_timing_info == NULL) {
		DE_WRN("get_video_timing_info func is null\n");
		return DIS_FAIL;
	}

	p_vdpop->vdpo_func.tv_get_video_timing_info(p_vdpop->vdpo_index,
						    &(p_vdpop->video_info));
	if (p_vdpop->video_info == NULL) {
		DE_WRN("video info is null\n");
		return DIS_FAIL;
	}

	memcpy(&p_vdpo->timings, p_vdpop->video_info,
	       sizeof(struct disp_video_timings));

	vdpo_calc_judge_line(p_vdpo);

	if (mgr->enable)
		mgr->enable(mgr);

	disp_sys_register_irq(p_vdpop->irq_no, 0, disp_vdpo_event_proc,
			      (void *)p_vdpo, 0, 0);
	disp_sys_enable_irq(p_vdpop->irq_no);

	ret = vdpo_clk_enable(p_vdpo);
	if (ret != 0) {
		DE_WRN("fail to enable vdpo's clock\n");
		goto EXIT;
	}

	disp_al_vdpo_cfg(p_vdpo->hwdev_index, p_vdpop->vdpo_index,
			 p_vdpop->video_info);
	disp_al_vdpo_enable(p_vdpo->hwdev_index,
			    p_vdpop->vdpo_index); /* enable irq herer */
#if !defined(DISP2_VDPO_USE_TCON_S_IRQ)
	p_vdpop->vdpo_func.tv_irq_enable(p_vdpop->vdpo_index, 0, 1);
#endif /* endif DISP2_VDPO_USE_TCON_S_IRQ */


	if (p_vdpop->vdpo_func.tv_enable != NULL)
		p_vdpop->vdpo_func.tv_enable(p_vdpop->vdpo_index);
	else
		DE_WRN("vdpo enable func is NULL\n");

	spin_lock_irqsave(&g_vdpo_data_lock, flags);
	p_vdpop->enabled = 1;
	spin_unlock_irqrestore(&g_vdpo_data_lock, flags);

EXIT:
	return ret;
}

s32 disp_vdpo_sw_enable(struct disp_device *p_vdpo)
{
	struct disp_manager *mgr = NULL;
	unsigned long flags;
	struct disp_vdpo_private_data *p_vdpop = disp_vdpo_get_priv(p_vdpo);

	if (!p_vdpo || !p_vdpop) {
		DE_WRN("p_vdpo init null hdl!\n");
		return DIS_FAIL;
	}

	mgr = p_vdpo->manager;
	if (!mgr) {
		DE_WRN("vdpo%d's mgr is NULL\n", p_vdpo->disp);
		return DIS_FAIL;
	}

	if (p_vdpop->enabled == 1) {
		DE_WRN("vdpo%d is already open\n", p_vdpo->disp);
		return DIS_FAIL;
	}

	if (p_vdpop->vdpo_func.tv_get_video_timing_info == NULL) {
		DE_WRN("get_video_timing_info func is null\n");
		return DIS_FAIL;
	}

	if (p_vdpop->vdpo_func.tv_irq_enable == NULL) {
		DE_WRN("vdpo_func.tv_irq_enable is null\n");
		return DIS_FAIL;
	}

	p_vdpop->vdpo_func.tv_get_video_timing_info(p_vdpop->vdpo_index,
						    &(p_vdpop->video_info));

	if (p_vdpop->video_info == NULL) {
		DE_WRN("video info is null\n");
		return DIS_FAIL;
	}

	if (p_vdpop->vdpo_func.tv_irq_enable == NULL) {
		DE_WRN("vdpo_func.tv_irq_enable is null\n");
		return DIS_FAIL;
	}

	memcpy(&p_vdpo->timings, p_vdpop->video_info,
	       sizeof(struct disp_video_timings));

	vdpo_calc_judge_line(p_vdpo);

	if (mgr->sw_enable)
		mgr->sw_enable(mgr);

#if defined(DISP2_VDPO_USE_TCON_S_IRQ)
	disp_al_device_disable_irq(p_vdpo->hwdev_index);
#else
	p_vdpop->vdpo_func.tv_irq_enable(p_vdpop->vdpo_index, 0, 0);
#endif
	disp_sys_register_irq(p_vdpop->irq_no, 0, disp_vdpo_event_proc,
			      (void *)p_vdpo, 0, 0);
	disp_sys_enable_irq(p_vdpop->irq_no);
#if defined(DISP2_VDPO_USE_TCON_S_IRQ)
	disp_al_device_enable_irq(p_vdpo->hwdev_index);
#else
	p_vdpop->vdpo_func.tv_irq_enable(p_vdpop->vdpo_index, 0, 1);
#endif

#if !defined(CONFIG_COMMON_CLK_ENABLE_SYNCBOOT)
	if (vdpo_clk_enable(p_vdpo) != 0)
		return -1;
#endif

	spin_lock_irqsave(&g_vdpo_data_lock, flags);
	p_vdpop->enabled = 1;
	spin_unlock_irqrestore(&g_vdpo_data_lock, flags);
	return 0;
}

static s32 disp_vdpo_get_status(struct disp_device *p_vdpo)
{
	if (!p_vdpo) {
		DE_WRN("NULL hdl!\n");
		return 0;
	}

	return disp_al_device_get_status(p_vdpo->hwdev_index);
}

static s32 disp_vdpo_set_static_config(struct disp_device *p_vdpo,
			       struct disp_device_config *config)
{
	return disp_vdpo_set_mode(p_vdpo, config->mode);
}

static s32 disp_vdpo_get_static_config(struct disp_device *p_vdpo,
				       struct disp_device_config *config)
{
	int ret = 0;
	struct disp_vdpo_private_data *p_vdpop = disp_vdpo_get_priv(p_vdpo);

	if (!p_vdpo || !p_vdpop) {
		DE_WRN("NULL hdl!\n");
		ret = -1;
		goto exit;
	}

	config->type = p_vdpo->type;
	config->mode = p_vdpop->mode;
	if (p_vdpop->vdpo_func.tv_get_input_csc == NULL)
		return DIS_FAIL;
	config->format =
	    p_vdpop->vdpo_func.tv_get_input_csc(p_vdpop->vdpo_index);

exit:
	return ret;
}

/**
 * @name       :disp_init_vdpo
 * @brief      :register vdpo device
 * @param[IN]  :para init parameter
 * @param[OUT] :none
 * @return     :0 if success otherwise fail
 */
s32 disp_init_vdpo(struct disp_bsp_init_para *para)
{
	char compat[32] = {0};
	struct device_node *node;
	u32 num_devices_support_vdpo = 0;
	s32 ret = 0;
	u32 disp = 0;
	const char *str;
	struct disp_device *p_vdpo;
	struct disp_vdpo_private_data *p_vdpop;
	u32 num_devices;
	u32 hwdev_index = 0; /* the index of tcon */
	u32 num_vdpo = 0;
	u32 vdpo_index = 0;

	spin_lock_init(&g_vdpo_data_lock);

	snprintf(compat, sizeof(compat), "allwinner,sunxi-vdpo");
	node = of_find_compatible_node(NULL, NULL, compat);
	if (!node) {
		DE_WRN("Can not get the dts of vdpo\n");
		goto exit;
	}

	ret = of_property_read_string(node, "status", &str);
	if (ret || strcmp(str, "okay")) {
		DE_WRN("vdpo status is not okay\n");
		goto exit;
	}
	g_vdpo_used = 1;

	DE_INF("%s\n", __func__);

	num_devices = bsp_disp_feat_get_num_devices(); /* get number of tcon */

	/* get the total number of tcon that is support vdpo */
	for (hwdev_index = 0; hwdev_index < num_devices; hwdev_index++) {
		if (bsp_disp_feat_is_supported_output_types(
			hwdev_index, DISP_OUTPUT_TYPE_VDPO))
			++num_devices_support_vdpo;
	}

	num_vdpo = bsp_disp_feat_get_num_vdpo();
	if (!num_vdpo) {
		DE_WRN("no vdpo device found!\n");
		goto exit;
	}

	g_pvdpo_devices =
	    kmalloc(sizeof(struct disp_device) * num_devices_support_vdpo,
		    GFP_KERNEL | __GFP_ZERO);

	if (g_pvdpo_devices == NULL) {
		DE_WRN("malloc memory for g_pvdpo_devices fail!\n");
		goto malloc_err;
	}

	g_pvdpo_private =
	    kmalloc_array(num_devices_support_vdpo, sizeof(*p_vdpop),
			  GFP_KERNEL | __GFP_ZERO);
	if (g_pvdpo_private == NULL) {
		DE_WRN("malloc memory for g_pvdpo_private fail!\n");
		goto malloc_err;
	}

	disp = 0;
	for (hwdev_index = 0; hwdev_index < num_devices; hwdev_index++) {
		if (!bsp_disp_feat_is_supported_output_types(
			hwdev_index, DISP_OUTPUT_TYPE_VDPO)) {
			DE_WRN("screen %d don't support VDPO!\n", hwdev_index);
			continue;
		}
		p_vdpo                     = &g_pvdpo_devices[disp];
		p_vdpop                    = &g_pvdpo_private[disp];
		p_vdpo->priv_data          = (void *)p_vdpop;

		p_vdpo->disp               = disp;
		p_vdpo->hwdev_index        = hwdev_index;
		sprintf(p_vdpo->name, "vdpo%d", disp);
		p_vdpo->type               = DISP_OUTPUT_TYPE_VDPO;
		p_vdpop->mode              = DISP_TV_MOD_720P_60HZ;
#if defined(DISP2_VDPO_USE_TCON_S_IRQ)
		p_vdpop->irq_no = para->irq_no[DISP_MOD_LCD0 + hwdev_index];
#else
		p_vdpop->irq_no = para->irq_no[DISP_MOD_VDPO];
#endif /* endif DISP2_VDPO_USE_TCON_S_IRQ */
		p_vdpop->vdpo_index = vdpo_index;
		/* tcon clk */
		p_vdpop->clk = para->mclk[DISP_MOD_LCD0 + hwdev_index];
		/* function register */
		p_vdpo->set_manager        = disp_device_set_manager;
		p_vdpo->unset_manager      = disp_device_unset_manager;
		p_vdpo->get_resolution     = disp_device_get_resolution;
		p_vdpo->get_timings        = disp_device_get_timings;
		p_vdpo->is_interlace       = disp_device_is_interlace;
		p_vdpo->init               = disp_vdpo_init;
		p_vdpo->exit               = disp_vdpo_exit;
		p_vdpo->set_tv_func        = disp_vdpo_set_func;
		p_vdpo->enable             = disp_vdpo_enable;
		p_vdpo->disable            = disp_vdpo_disable;
		p_vdpo->is_enabled         = disp_vdpo_is_enabled;
		p_vdpo->sw_enable          = disp_vdpo_sw_enable;
		p_vdpo->check_if_enabled   = disp_vdpo_check_if_enabled;
		p_vdpo->check_support_mode = disp_vdpo_check_support_mode;
		p_vdpo->get_input_csc      = disp_vdpo_get_input_csc;
		p_vdpo->suspend            = disp_vdpo_suspend;
		p_vdpo->resume             = disp_vdpo_resume;
		p_vdpo->get_fps            = disp_vdpo_get_fps;
		p_vdpo->set_mode           = disp_vdpo_set_mode;
		p_vdpo->get_mode           = disp_vdpo_get_mode;
		p_vdpo->set_static_config = disp_vdpo_set_static_config;
		p_vdpo->get_static_config = disp_vdpo_get_static_config;
		p_vdpo->get_status = disp_vdpo_get_status;
		p_vdpo->init(p_vdpo);

		disp_device_register(p_vdpo);
		++disp;
		if (vdpo_index < num_vdpo-1)
			++vdpo_index;
	}
	return 0;

malloc_err:
	if (g_pvdpo_devices != NULL)
		kfree(g_pvdpo_devices);
	if (g_pvdpo_private != NULL)
		kfree(g_pvdpo_private);
	g_pvdpo_devices = NULL;
	g_pvdpo_private = NULL;
exit:
	return -1;
}

s32 disp_vdpo_set_config(struct disp_device *p_vdpo,
			 struct disp_vdpo_config *p_cfg)
{
	s32 ret = -1;
	struct disp_vdpo_private_data *p_vdpop = disp_vdpo_get_priv(p_vdpo);

	if (p_vdpop->vdpo_func.vdpo_set_config)
		ret = p_vdpop->vdpo_func.vdpo_set_config(p_vdpop->vdpo_index,
							 p_cfg);

	return ret;
}
/**
 * @name       :disp_exit_vdpo
 * @brief      :exit vdpo module
 * @param[IN]  :
 * @param[OUT] :
 * @return     :
 */
s32 disp_exit_vdpo(void)
{
	s32 ret = 0;

	DE_WRN("\n");
	/* TODO */
	return ret;
}
#endif /* endif SUPPORT_VDPO */
