/*
 * Copyright (C) 2015 Allwinnertech, z.q <zengqi@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#include "disp_eink.h"

#ifdef SUPPORT_EINK
#include <linux/sched.h>

/* #define __EINK_DEBUG__ */
/* #define __EINK_TEST__ */
#define MAX_EINK_ENGINE 1

#ifdef __EINK_DEBUG__
#define USED 1
#define LBL 4
#define LEL 44
#define LSL 10
#define FBL 4
#define FEL 12
#define FSL 4
#define WIDTH 800
#define HEIGHT 600
#endif

#ifdef EINK_FLUSH_TIME_TEST
struct timeval wb_end_timer, flush_start_timer, open_tcon_timer;
struct timeval flush_end_timer, test_start, test_end, en_lcd, dis_lcd;
struct timeval index_hard_timer, start_decode_timer;
struct timeval en_decode[3];
struct timeval fi_decode[3];
unsigned int t3_f3[3] = { 0, 0, 0 };
static int decode_task_t;
static int decode_t;

unsigned int lcd_t1, lcd_t2, lcd_t3, lcd_t4, lcd_t5, lcd_pin, lcd_po, lcd_tcon;
struct timeval ioctrl_start_timer;
unsigned int t1 = 0, t2 = 0, t3 = 0, t4 = 0;
unsigned int t3_1 = 0, t3_2 = 0, t3_3 = 0, t2_1 = 0, t2_2 = 0;
#endif

#ifdef __EINK_TEST__
static int decount;
static int wacount;
static int emcount;
static int qcount;
#endif

static struct disp_eink_manager *eink_manager;
static struct eink_private *eink_private_data;
static int suspend;
static unsigned char index_finish_flag;
static int display_pre_finish_flag;
int display_finish_flag = 1;

static int __write_edma_first(struct disp_eink_manager *manager);
static int __write_edma_second(struct disp_eink_manager *manager);
static int __write_edma(struct disp_eink_manager *manager);

struct disp_eink_manager *disp_get_eink_manager(unsigned int disp)
{
	return &eink_manager[disp];
}

unsigned int get_temperature(struct disp_eink_manager *manager)
{
	unsigned int temp = 28;

	if (manager)
		temp = manager->get_temperature(manager);
	else
		__debug("eink manager is null\n");

	return temp;
}

struct eink_private *eink_get_priv(struct disp_eink_manager *manager)
{
	return (manager != NULL) ? (&eink_private_data[manager->disp]) : NULL;
}

int __eink_clk_init(struct disp_eink_manager *manager)
{
	return 0;
}

int __eink_clk_disable(struct disp_eink_manager *manager)
{
	int ret = 0;

	if (manager->private_data->eink_clk)
		clk_disable(manager->private_data->eink_clk);

	if (manager->private_data->edma_clk)
		clk_disable(manager->private_data->edma_clk);

	return ret;
}

int __eink_clk_enable(struct disp_eink_manager *manager)
{
	int ret = 0;

	if (manager->private_data->eink_clk)
		ret = clk_prepare_enable(manager->private_data->eink_clk);

	if (manager->private_data->edma_clk)
		ret = clk_prepare_enable(manager->private_data->edma_clk);

	return ret;
}

void __clear_wavedata_buffer(struct wavedata_queue *queue)
{
	int i = 0;
	unsigned long flags = 0;

	spin_lock_irqsave(&queue->slock, flags);

	queue->head = 0;
	queue->tail = 0;
	for (i = 0; i < WAVE_DATA_BUF_NUM; i++)
		queue->wavedata_used[i] = false;

	spin_unlock_irqrestore(&queue->slock, flags);
}

#ifdef __EINK_TEST__
void __eink_clear_wave_data(struct disp_eink_manager *manager, int overlap)
{
	__clear_wavedata_buffer(&(manager->private_data->wavedata_ring_buffer));
}

int __eink_debug_decode(struct disp_eink_manager *manager, int enable)
{
	int ret = 0;

#ifdef EINK_FLUSH_TIME_TEST
	do_gettimeofday(&test_start);
	msleep(300);
	do_gettimeofday(&test_end);
#endif

	return ret;
}

static bool __is_wavedata_buffer_full(struct wavedata_queue *queue)
{
	bool ret;
	unsigned long flags = 0;

	spin_lock_irqsave(&queue->slock, flags);

	ret =
	    ((queue->head + 1) % WAVE_DATA_BUF_NUM == queue->tail) ?
	    true : false;

	spin_unlock_irqrestore(&queue->slock, flags);

	return ret;

}

static bool __is_wavedata_buffer_empty(struct wavedata_queue *queue)
{
	bool ret;
	unsigned long flags = 0;

	spin_lock_irqsave(&queue->slock, flags);

	ret = (queue->head == queue->tail) ? true : false;

	spin_unlock_irqrestore(&queue->slock, flags);

	return ret;

}
#endif/* __EINK_TEST__ */

static void __eink_get_sys_config(u32 disp, struct eink_init_param *eink_param)
{
	int value = 1;
	char primary_key[20];
	int ret;

	sprintf(primary_key, "lcd%d", disp);

	ret = disp_sys_script_get_item(primary_key, "lcd_used", &value, 1);
	if (ret == 1)
		eink_param->used = value;

	if (eink_param->used == 0)
		return;

	ret = disp_sys_script_get_item(primary_key, "eink_bits", &value, 1);
	if (ret == 1)
		eink_param->eink_bits = value;

	ret = disp_sys_script_get_item(primary_key, "eink_mode", &value, 1);
	if (ret == 1)
		eink_param->eink_mode = value;

	ret = disp_sys_script_get_item(primary_key, "eink_lbl", &value, 1);
	if (ret == 1)
		eink_param->timing.lbl = value;

	ret = disp_sys_script_get_item(primary_key, "eink_lel", &value, 1);
	if (ret == 1)
		eink_param->timing.lel = value;

	ret = disp_sys_script_get_item(primary_key, "eink_lsl", &value, 1);
	if (ret == 1)
		eink_param->timing.lsl = value;

	ret = disp_sys_script_get_item(primary_key, "eink_fbl", &value, 1);
	if (ret == 1)
		eink_param->timing.fbl = value;

	ret = disp_sys_script_get_item(primary_key, "eink_fel", &value, 1);
	if (ret == 1)
		eink_param->timing.fel = value;

	ret = disp_sys_script_get_item(primary_key, "eink_fsl", &value, 1);
	if (ret == 1)
		eink_param->timing.fsl = value;

	ret = disp_sys_script_get_item(primary_key, "eink_width", &value, 1);
	if (ret == 1)
		eink_param->timing.width = value;

	ret = disp_sys_script_get_item(primary_key, "eink_height", &value, 1);
	if (ret == 1)
		eink_param->timing.height = value;

	ret =
	    disp_sys_script_get_item(primary_key, "eink_path",
				     (int *)&eink_param->wavefile_path, 2);
	if (ret != 2)
		__wrn("get eink path fail!\n");
}

static int __eink_interrupt_proc(int irq, void *parg)
{
	struct disp_eink_manager *manager;
	unsigned int disp;
	int ret = -1;

	manager = (struct disp_eink_manager *)parg;
	if (!manager)
		return DISP_IRQ_RETURN;

	disp = manager->disp;
	ret = disp_al_eink_irq_query(manager->disp);

	/* query irq, 0 is decode, 1 is calculate index. */
	if (ret == 1) {
		index_finish_flag = 1;
		goto out;
	} else if (ret == 0) {
		schedule_work(&manager->decode_work);
		goto out;
	}

out:

	return DISP_IRQ_RETURN;
}

/* return a physic address for tcon
 * used to display wavedata,then
 * dequeue wavedata buffer.
*/
static void *__request_buffer_for_display(struct wavedata_queue *queue)
{
	void *ret;
	unsigned long flags = 0;
	bool is_wavedata_buf_empty;

	spin_lock_irqsave(&queue->slock, flags);

	is_wavedata_buf_empty = (queue->head == queue->tail) ? true : false;
	if (is_wavedata_buf_empty) {
		ret = NULL;
		goto out;
	}

	ret = (void *)queue->wavedata_paddr[queue->tail];

out:

	spin_unlock_irqrestore(&queue->slock, flags);

	return ret;
}

/* return a physic address for eink
*  engine used to decode one frame,
*  then queue wavedata buffer.
*/
static void *__request_buffer_for_decode(struct wavedata_queue *queue)
{
	void *ret;
	unsigned long flags = 0;
	bool is_wavedata_buf_full, is_used;

	spin_lock_irqsave(&queue->slock, flags);

	is_wavedata_buf_full =
	    ((queue->head + 1) % WAVE_DATA_BUF_NUM == queue->tail) ?
	    true : false;
	is_used = queue->wavedata_used[queue->head];
	if (is_wavedata_buf_full || is_used) {
		ret = NULL;
		goto out;
	}

	ret = (void *)queue->wavedata_paddr[queue->head];

out:
	spin_unlock_irqrestore(&queue->slock, flags);
	return ret;
}

static s32 __queue_wavedata_buffer(struct wavedata_queue *queue)
{
	int ret = 0;
	unsigned long flags = 0;
	bool is_wavedata_buf_full;

	spin_lock_irqsave(&queue->slock, flags);

	is_wavedata_buf_full =
	    ((queue->head + 1) % WAVE_DATA_BUF_NUM == queue->tail) ?
	    true : false;

	if (is_wavedata_buf_full) {
		ret = -EBUSY;
		goto out;
	}
	/* set used status true */
	queue->wavedata_used[queue->head] = true;

	queue->head = (queue->head + 1) % WAVE_DATA_BUF_NUM;

out:

#ifdef __EINK_TEST__
	qcount++;
#endif
	spin_unlock_irqrestore(&queue->slock, flags);

	return ret;
}

static s32 __dequeue_wavedata_buffer(struct wavedata_queue *queue)
{
	int ret = 0;
	unsigned long flags = 0;
	bool is_wavedata_buf_empty;

	spin_lock_irqsave(&queue->slock, flags);

	is_wavedata_buf_empty = (queue->head == queue->tail) ? true : false;

	if (is_wavedata_buf_empty) {
		ret = -EBUSY;
		goto out;
	}
	queue->tail = (queue->tail + 1) % WAVE_DATA_BUF_NUM;

out:

#ifdef __EINK_TEST__
	decount++;
#endif
	spin_unlock_irqrestore(&queue->slock, flags);

	return ret;
}

static s32 __clean_used_wavedata_buffer(struct wavedata_queue *queue)
{
	int ret = 0;
	unsigned long flags = 0;

	spin_lock_irqsave(&queue->slock, flags);

	if (queue->tail >= 2) {
		queue->wavedata_used[queue->tail - 2] = false;
	} else {
		queue->wavedata_used[queue->tail + WAVE_DATA_BUF_NUM - 2] =
		    false;
	}

	spin_unlock_irqrestore(&queue->slock, flags);

	return ret;
}

static int index_err;
static s32 eink_calculate_index_data(struct disp_eink_manager *manager)
{
	unsigned long flags = 0;
	struct eink_8bpp_image *last_image;
	struct eink_8bpp_image *current_image;
	unsigned long old_index_data_paddr = 0;
	unsigned long new_index_data_paddr = 0;
	unsigned int new_index = 0, old_index = 0;
	unsigned int t_new_index = 0, t_old_index = 0;
	int count = 0;

	/* last img */
	last_image = manager->buffer_mgr->get_last_image(manager->buffer_mgr);
	/* cur img */
	current_image =
	    manager->buffer_mgr->get_current_image(manager->buffer_mgr);

	if (!last_image || !current_image) {
		__wrn("image paddr is NULL!\n");
		return -EINVAL;
	}

	spin_lock_irqsave(&manager->private_data->slock, flags);

	t_new_index = new_index = manager->private_data->new_index;
	t_old_index = old_index = manager->private_data->old_index;

	if (new_index > 1 || old_index > 1 || new_index != old_index) {
		__wrn("index larger then 1,new_index=%d,old_index=%d\n",
		      new_index, old_index);
		spin_unlock_irqrestore(&manager->private_data->slock, flags);
		return -EINVAL;
	}

	if (old_index == new_index)
		manager->private_data->new_index =
		    1 - manager->private_data->old_index;

	old_index = manager->private_data->old_index;
	new_index = manager->private_data->new_index;

	old_index_data_paddr =
	    (unsigned long)manager->private_data->index_paddr[old_index];
	new_index_data_paddr =
	    (unsigned long)manager->private_data->index_paddr[new_index];

	__debug
	    ("new index=%d,old_index=%d, old_index_data_paddr=%p, new_index_data_paddr=%p\n",
	     manager->private_data->new_index, manager->private_data->old_index,
	     (void *)old_index_data_paddr, (void *)new_index_data_paddr);

	spin_unlock_irqrestore(&manager->private_data->slock, flags);

#ifdef EINK_FLUSH_TIME_TEST
	do_gettimeofday(&index_hard_timer);
#endif

	disp_al_eink_start_calculate_index(manager->disp,
					   old_index_data_paddr,
					   new_index_data_paddr,
					   last_image, current_image);
						/* calculate index */

	/* check hardware status,if calculate over, then continue,
	 *  otherwise wait for status,if timeout, throw warning and quit.
	 */

	while ((index_finish_flag != 1) && (count < 200)) {
		count++;

		/* it may fix by different param by hardware.
		 * if too less,the first frame index calc err.
		 * at this time, no use msleep.
		 */
		udelay(300);
	}

	if ((count >= 200) && (index_finish_flag != 1)) {
		__wrn("calculate index data is wrong!\n");

		spin_lock_irqsave(&manager->private_data->slock, flags);
		manager->private_data->new_index = t_new_index;
		manager->private_data->old_index = t_old_index;
		spin_unlock_irqrestore(&manager->private_data->slock, flags);
		eink_irq_query_index();
		index_finish_flag = 0;
		index_err = 1;
		return -EBUSY;
	}
	index_err = 0;
	index_finish_flag = 0;
	if (current_image->window_calc_enable)
		disp_al_get_update_area(manager->disp,
					&current_image->update_area);

#ifdef __EINK_TEST__
	__debug("calc en=%d, flash mode = %d\n",
		current_image->window_calc_enable, current_image->flash_mode);
	__debug("xtop=%d,ytop=%d,xbot=%d,ybot=%d\n",
		current_image->update_area.x_top,
		current_image->update_area.y_top,
		current_image->update_area.x_bottom,
		current_image->update_area.y_bottom);
#endif

	/* if calculate index success, then switch the
	 * index double buffer,set index fresh flag.
	 */
	spin_lock_irqsave(&manager->private_data->slock, flags);

	manager->private_data->index_fresh = true;

	spin_unlock_irqrestore(&manager->private_data->slock, flags);

#ifdef EINK_FLUSH_TIME_TEST
	do_gettimeofday(&flush_start_timer);
	t2 = (flush_start_timer.tv_sec - wb_end_timer.tv_sec) * 1000000 +
	    (flush_start_timer.tv_usec - wb_end_timer.tv_usec);
	t2_1 =
	    (index_hard_timer.tv_sec - wb_end_timer.tv_sec) * 1000000 +
	    (index_hard_timer.tv_usec - wb_end_timer.tv_usec);
	t2_2 =
	    (flush_start_timer.tv_sec - index_hard_timer.tv_sec) * 1000000 +
	    (flush_start_timer.tv_usec - index_hard_timer.tv_usec);
#endif

	return 0;

}

static int start_decode(struct disp_eink_manager *manager,
			unsigned long wavedata_paddr, unsigned long index_paddr)
{
	struct eink_private *data;
	struct eink_init_param param;
	int ret = 0;

	data = eink_get_priv(manager);
	memcpy((void *)&param, (void *)&data->param,
	       sizeof(struct eink_init_param));
#ifdef EINK_FLUSH_TIME_TEST
	if (decode_t == 0) {
		do_gettimeofday(&start_decode_timer);
		t3_1 =
		    (start_decode_timer.tv_sec -
		     flush_start_timer.tv_sec) * 1000000 +
		    (start_decode_timer.tv_usec - flush_start_timer.tv_usec);
	}
#endif
	ret =
	    disp_al_eink_start_decode(manager->disp, index_paddr,
				      wavedata_paddr, &param);

#ifdef EINK_FLUSH_TIME_TEST
	if (decode_t <= 2) {
		do_gettimeofday(&en_decode[decode_t]);
		decode_t++;
	}
#endif

	return ret;
}

static int current_frame;
extern s32 disp_lcd_tcon_disable(struct disp_device *lcd);

int eink_display_one_frame(struct disp_eink_manager *manager)
{
	unsigned long flags = 0;
	int ret = 0;
	int index = 0;
	struct disp_device *plcd = NULL;

	spin_lock_irqsave(&manager->private_data->slock, flags);

	manager->private_data->fresh_frame_index++;

	index = manager->private_data->fresh_frame_index;

	if (manager->private_data->fresh_frame_index ==
	    (manager->private_data->total_frame)) {

		manager->private_data->fresh_frame_index = 0;
		/* manager->private_data->total_frame = 0; */
		__clear_wavedata_buffer(&(manager->private_data->wavedata_ring_buffer));
#ifdef __EINK_TEST__

		unsigned int head = 0, tail = 0;
		unsigned long flags1 = 0;

		spin_lock_irqsave(&manager->private_data->wavedata_ring_buffer.
				  slock, flags1);
		tail = manager->private_data->wavedata_ring_buffer.tail;
		head = manager->private_data->wavedata_ring_buffer.head;
		spin_unlock_irqrestore(&manager->private_data->
				       wavedata_ring_buffer.slock, flags1);
		__debug("fin:tai=%d,hed=%d,idx=%d,dc=%d,qc=%d,ec=%d,tf=%d\n",
			tail, head, index, decount, qcount, emcount,
			manager->private_data->total_frame);
		decount = wacount = emcount = qcount = 0;
#endif
		current_frame = 0;

		/* __eink_clk_disable(manager); */
		display_pre_finish_flag = 1;
		plcd = disp_device_find(manager->disp, DISP_OUTPUT_TYPE_LCD);
		disp_lcd_tcon_disable(plcd);
		/* colse eink panel */
		schedule_work(&plcd->close_eink_panel_work);

#ifdef EINK_FLUSH_TIME_TEST
		do_gettimeofday(&flush_end_timer);
		t4 = (flush_end_timer.tv_sec -
		      open_tcon_timer.tv_sec) * 1000000 +
		    (flush_end_timer.tv_usec - open_tcon_timer.tv_usec);
		__debug
		    ("us:t1 = %u,t2 = %u,t2_1 = %u,t2_2 = %u,t3 = %u,t4 = %u\n",
		     t1, t2, t2_1, t2_2, t3, t4);
		__debug("us:t3_1 = %u, t3_2 = %u, t3_3 = %u\n", t3_1, t3_2,
			t3_3);
		__debug("us:t3_f1 = %u, t3_f2 = %u, t3_f3 = %u\n", t3_f3[0],
			t3_f3[1], t3_f3[2]);
		__debug
		    ("us:lcd1=%u,lcd2=%u,lcd3=%u,lcd4=%u,lcd_t5=%u,lcd_po=%u,lcd_pin=%u,lcd_tcon=%u\n",
		     lcd_t1, lcd_t2, lcd_t3, lcd_t4, lcd_t5, lcd_po, lcd_pin,
		     lcd_tcon);

		t1 = t2 = t3 = t4 = t3_1 = t3_2 = t3_3 = t3_f3[0] = t3_f3[1] =
		    t3_f3[2] = 0;
		decode_task_t = 0;
		decode_t = 0;
#endif
	} else if (manager->private_data->fresh_frame_index <
		   (manager->private_data->total_frame - 1)) {
		tasklet_schedule(&manager->sync_tasklet);
	}

	spin_unlock_irqrestore(&manager->private_data->slock, flags);

	return ret;
}

static int eink_decode_finish(struct disp_eink_manager *manager)
{
	unsigned long flags = 0;
	int ret = 0;

	spin_lock_irqsave(&manager->private_data->slock, flags);

	if (manager->private_data->total_frame ==
	    manager->private_data->decode_frame_index) {
		__debug("decode finish!,tot=%d, frame=%d, fresh_frame=%d.\n",
			manager->private_data->total_frame,
			manager->private_data->decode_frame_index,
			manager->private_data->fresh_frame_index);

		manager->private_data->decode_frame_index = 0;
		ret = 1;
		goto out;
	}

out:
	spin_unlock_irqrestore(&manager->private_data->slock, flags);
	__debug("frame_index=%d, total_index=%d\n",
		manager->private_data->decode_frame_index,
		manager->private_data->total_frame);

	return ret;
}

void __sync_task(unsigned long disp)
{
	struct disp_eink_manager *manager;
	struct eink_private *private_data;
	int cur_line = 0;
	static int start_delay;

	start_delay = disp_al_lcd_get_start_delay(disp, NULL);
	manager = disp_get_eink_manager((unsigned int)disp);
	private_data = manager->private_data;

	manager->tcon_flag = 0;
	cur_line = disp_al_lcd_get_cur_line(disp, NULL);
	__clean_used_wavedata_buffer(&private_data->wavedata_ring_buffer);

	while (cur_line < start_delay && !display_pre_finish_flag)
		cur_line = disp_al_lcd_get_cur_line(disp, NULL);
	__write_edma(manager);
}

/* #define DEBUG_CHANGE_DATA */
#ifdef DEBUG_CHANGE_DATA
static u32 decode_phy_addr;
static u32 *decode_virt_addr;

#define START_OFFSET 2336	/* (9*258+14) */
#define END_OFFSET  157074	/* (609*258-44 - 4) */
#endif

void eink_decode_task(struct work_struct *work)
{
	struct disp_eink_manager *manager;
	bool image_buffer_empty;
	unsigned int temperature;
	int insert = 0;
	unsigned long flags = 0;
	unsigned long index_buf_paddr = 0, wavedata_paddr = 0;
	unsigned int tframes = 0;
	unsigned int new_index;
	int frame = 0;
	int count = 0;
	static int first = 1;

#ifdef EINK_FLUSH_TIME_TEST
	if (decode_task_t <= 2) {
		do_gettimeofday(&fi_decode[decode_task_t]);
		t3_f3[decode_task_t] =
		    (fi_decode[decode_task_t].tv_sec -
		     en_decode[decode_task_t].tv_sec) * 1000000 +
		    (fi_decode[decode_task_t].tv_usec -
		     en_decode[decode_task_t].tv_usec);
		decode_task_t++;
	}
#endif

	manager = disp_get_eink_manager((unsigned int)0);
	temperature = get_temperature(manager);
	__queue_wavedata_buffer(&manager->private_data->wavedata_ring_buffer);
	/* update pipeline */
	insert =
	    manager->pipeline_mgr->update_pipeline_list(manager->pipeline_mgr,
							temperature, &tframes);
	spin_lock_irqsave(&manager->private_data->slock, flags);

	frame = manager->private_data->decode_frame_index++;
	if (insert == 1) {
		unsigned int need_frames = 0;

		need_frames = manager->private_data->total_frame -
		    manager->private_data->decode_frame_index;

		if (need_frames <= tframes) {
			manager->private_data->total_frame =
			    tframes + manager->private_data->decode_frame_index;
			__debug("need = %d, totoal=%d\n", need_frames,
				manager->private_data->total_frame);
		}
	}

	spin_unlock_irqrestore(&manager->private_data->slock, flags);

	if (frame == 2) {
		struct disp_device *plcd = NULL;

		__write_edma_first(manager);
#ifdef EINK_FLUSH_TIME_TEST
		do_gettimeofday(&en_lcd);
		t3_2 =
		    (en_lcd.tv_sec - start_decode_timer.tv_sec) * 1000000 +
		    (en_lcd.tv_usec - start_decode_timer.tv_usec);
#endif
		/* use lcd_enable now, reserve lcd simple open method. */

		plcd = disp_device_find(manager->disp, DISP_OUTPUT_TYPE_LCD);

		if (first) {
			display_pre_finish_flag = 0;
			plcd->enable(plcd);/* lcd enable */
			/* first = 0; */
		} else {
			tcon0_simple_open(0);
		}

#ifdef EINK_FLUSH_TIME_TEST
		do_gettimeofday(&dis_lcd);
		t3_3 =
		    (dis_lcd.tv_sec - en_lcd.tv_sec) * 1000000 +
		    (dis_lcd.tv_usec - en_lcd.tv_usec);
		t3 = (dis_lcd.tv_sec - flush_start_timer.tv_sec) * 1000000 +
		    (dis_lcd.tv_usec - flush_start_timer.tv_usec);
#endif

		__write_edma_second(manager);
	}

	/* debug by changing data. */
#ifdef DEBUG_CHANGE_DATA
	u16 *point = decode_virt_addr;
	u8 data_id = 0;
	u16 *start_point = NULL, *end_point = NULL;
	u16 start_data = 0, end_data = 0;

	start_point = (point + START_OFFSET);
	end_point = (point + END_OFFSET);
	for (data_id = 0; data_id < 4; data_id++) {
		start_data = *(start_point + data_id) & 0xff;
		start_data = start_data | (frame << 8);
		*(start_point + data_id) = start_data;
	}

	for (data_id = 0; data_id < 4; data_id++) {
		end_data = *(end_point + data_id) & 0xff;
		end_data = end_data | (frame << 8);
		*(end_point + data_id) = end_data;
	}
#endif
	/* decode finish */
	if (eink_decode_finish(manager)) {
		spin_lock_irqsave(&manager->private_data->slock, flags);
		manager->private_data->index_fresh = false;
		spin_unlock_irqrestore(&manager->private_data->slock, flags);

		image_buffer_empty =
		    manager->buffer_mgr->is_empty(manager->buffer_mgr);
		if (image_buffer_empty) {
			/* disable eink engine. */
			manager->disable(manager);
		}
	} else {
		spin_lock_irqsave(&manager->private_data->slock, flags);

		if (insert == 1) {
			/*
			 * insert a new pipeline to list,
			 * it need switch index buffer.
			 */
			new_index = manager->private_data->new_index;
			index_buf_paddr =
			    (unsigned long)manager->private_data->
			    index_paddr[new_index];
			manager->private_data->old_index = new_index;
		} else {
			new_index = manager->private_data->new_index;
			index_buf_paddr =
			    (unsigned long)manager->private_data->
			    index_paddr[new_index];
		}

		spin_unlock_irqrestore(&manager->private_data->slock, flags);

		wavedata_paddr =
		    (unsigned long)__request_buffer_for_decode(&manager->
						       private_data->
						       wavedata_ring_buffer);

		while ((!wavedata_paddr) && count < 100) {
			/* msleep(1); */
			usleep_range(500, 2000);
			wavedata_paddr =
			    (unsigned long)
			    __request_buffer_for_decode(&manager->private_data->
							wavedata_ring_buffer);
			count++;
		}
		if (count > 100) {
			__debug
			    ("wavedata_ring_buffer was full, stop decoding.\n");
			return;
		}
#ifdef DEBUG_CHANGE_DATA
		decode_virt_addr = phys_to_virt((unsigned long)wavedata_paddr);
#endif

		/* start decode */
		start_decode(manager, wavedata_paddr, index_buf_paddr);
		spin_lock_irqsave(&manager->private_data->slock, flags);

		if (insert == 1)
			manager->private_data->index_fresh = false;

		spin_unlock_irqrestore(&manager->private_data->slock, flags);
	}
}

static int eink_detect_fresh_thread(void *parg)
{
	unsigned long flags = 0;
	struct disp_eink_manager *manager;
	struct eink_8bpp_image *current_image;
	int overlap_num;
	unsigned long wavedata_paddr, index_paddr;
	unsigned int decode_frame_index;
	unsigned int tframes = 0;
	unsigned int temperature = 0;
	int ret = 0;

	volatile int display_finish;

	manager = (struct disp_eink_manager *)parg;
	for (;;) {
		struct eink_buffer_manager *buffer_mgr = manager->buffer_mgr;
		struct pipeline_manager *pl_mgr = manager->pipeline_mgr;
		struct eink_private *priv = manager->private_data;
		struct wavedata_queue *wavedata_ring_buffer =
					&priv->wavedata_ring_buffer;

		if (kthread_should_stop()) {
			while (!buffer_mgr->is_empty(buffer_mgr))
				buffer_mgr->dequeue_image(buffer_mgr);
			break;
		}

		temperature = get_temperature(manager);

		if (buffer_mgr->is_empty(buffer_mgr))
			continue;

		if (pl_mgr->used_list_status(pl_mgr) == 1)
			continue;

		/* if last index do not fresh, waiting for decode interrupt. */
		if (manager->private_data->index_fresh)
			continue;

		ret = eink_calculate_index_data(manager);
		if (ret) {
			buffer_mgr->dequeue_image(buffer_mgr);
			__wrn("index calc err, something is wrong.\n");
			if (buffer_mgr->is_empty(buffer_mgr)) {
				index_err = 0;
				manager->disable(manager);
				pl_mgr->clear_pipeline_list(pl_mgr);
				buffer_mgr->clear_image(buffer_mgr);
				__clear_wavedata_buffer(wavedata_ring_buffer);
				/* __eink_clk_disable(manager); */
				display_finish_flag = 1;
			}
			continue;
		}
#ifdef __EINK_TEST__
		while (!manager->flush_continue_flag) {
			/* msleep(5); */
			usleep_range(500, 5000);
		}
#endif
		current_image =
		    manager->buffer_mgr->get_current_image(manager->buffer_mgr);
		manager->pipeline_mgr->check_overlap(manager->pipeline_mgr,
					     current_image->update_area);/* check overlap */

		overlap_num =
		    pl_mgr->check_overlap_num(pl_mgr);/* overlap num */

#ifdef __EINK_TEST__
		if (overlap_num)
			__debug("OVERLAP!, overlap_num=%d\n", overlap_num);
		else
			__debug("NO OVERLAP.\n");
#endif

		while (overlap_num) {
			display_finish = display_finish_flag;
			while (!display_finish) {
				/* msleep(1); */
				usleep_range(500, 1500);
				display_finish = display_finish_flag;
			}
			/* msleep(1); */
			usleep_range(500, 1500);
			overlap_num =
			    pl_mgr->check_overlap_num(pl_mgr);
		}

		/* fix ,add timeout process,600ms later, then quit it. */

		/*
		 * get one free pipeline
		 * and set the current image update area to pipeline
		 */
		spin_lock_irqsave(&manager->private_data->slock, flags);

		decode_frame_index = manager->private_data->decode_frame_index;
		index_paddr =
		    (unsigned long)priv->index_paddr[priv->new_index];

		spin_unlock_irqrestore(&manager->private_data->slock, flags);

		/*
		 * if it's the first decode frame, config pipeline
		 * with the current update area, then enable it,
		 * else just config it, enabel it in next decode interrupt.
		 */
		current_image =
		    buffer_mgr->get_current_image(buffer_mgr);

		if (decode_frame_index == 0) {
			/*
			 * insert a new pipeline to list,
			 * it need switch index buffer. once start decode,
			 * it need decode until last frame display finish.
			 */
			display_finish = display_finish_flag;
			while (!display_finish) {
				/* msleep(1); */
				usleep_range(500, 1500);
				display_finish = display_finish_flag;
			}
			display_finish_flag = 0;

			pl_mgr->config_and_enable_one_pipeline(pl_mgr,
						   current_image->update_area,
						   current_image->update_mode,
						   temperature,
						   &tframes);

			spin_lock_irqsave(&manager->private_data->slock, flags);

			manager->private_data->total_frame = tframes;
			__debug("first total index = %d\n", tframes);
			manager->private_data->old_index =
			    manager->private_data->new_index;

			spin_unlock_irqrestore(&manager->private_data->slock,
					       flags);

			wavedata_paddr =
			    (unsigned long)
			    __request_buffer_for_decode(&priv->wavedata_ring_buffer);
			while (!wavedata_paddr) {
				wavedata_paddr =
				    (unsigned long)
				    __request_buffer_for_decode(&priv->wavedata_ring_buffer);
			}

			__debug("decode:[wd_paddr]=0x%p, [idx_paddr]=0x%p\n",
				(void *)wavedata_paddr, (void *)index_paddr);

#ifdef DEBUG_CHANGE_DATA
			decode_virt_addr = phys_to_virt(wavedata_paddr);
#endif

			start_decode(manager, wavedata_paddr, index_paddr);/* decode */

			spin_lock_irqsave(&manager->private_data->slock, flags);

			manager->private_data->index_fresh = false;

			spin_unlock_irqrestore(&priv->slock, flags);
		} else {
			pl_mgr->config_one_pipeline(pl_mgr,
						   current_image->update_area,
						   current_image->update_mode);
		}
		/* image ring buffer dequeue one buf */
		/* dequeue_image */
		manager->buffer_mgr->dequeue_image(manager->buffer_mgr);
	}

	return ret;
}

s32 eink_update_image(struct disp_eink_manager *manager,
		      struct disp_layer_config_inner *config,
		      unsigned int layer_num,
		      enum eink_update_mode mode, struct area_info update_area)
{
	int ret = 0;

	if (suspend)
		return -EBUSY;

	if (index_err)
		return -EAGAIN;

	manager->enable(manager);
	ret = manager->buffer_mgr->queue_image(manager->buffer_mgr, config,
					       layer_num, mode, update_area);

#ifdef EINK_FLUSH_TIME_TEST
	do_gettimeofday(&wb_end_timer);
	t1 = (wb_end_timer.tv_sec - ioctrl_start_timer.tv_sec) * 1000000 +
	    (wb_end_timer.tv_usec - ioctrl_start_timer.tv_usec);
#endif

	return ret;
}

static int eink_op_skip(struct disp_eink_manager *manager, u32 skip)
{
	return manager->pipeline_mgr->op_skip(manager->pipeline_mgr, skip);
}

static int first_enable = 1;

s32 eink_enable(struct disp_eink_manager *manager)
{
	unsigned long flags = 0;
	int ret = 0;
	struct eink_init_param param;
	struct eink_init_param *eink_param;

	suspend = 0;
	spin_lock_irqsave(&manager->private_data->slock, flags);

	if (manager->private_data->enable_flag) {
		spin_unlock_irqrestore(&manager->private_data->slock, flags);
		return ret;
	}

	spin_unlock_irqrestore(&manager->private_data->slock, flags);

#ifdef SUPPORT_WB
	manager->convert_mgr =
	    disp_get_format_manager(manager->convert_mgr->disp);
	/* WB enable */
	if (manager->convert_mgr)
		manager->convert_mgr->enable(manager->convert_mgr->disp);
	else
		__wrn("convert mgr is null.\n");
#endif
	eink_param = &manager->private_data->param;/* bits,data... */

	/* enable eink clk */
	if (first_enable)
		ret = __eink_clk_enable(manager);

	/* init eink and edma */
	memcpy((void *)&param, (void *)&manager->private_data->param,
	       sizeof(struct eink_init_param));
	ret = disp_al_eink_config(manager->disp, &param);
	ret = disp_al_edma_init(manager->disp, &param);

	/* load waveform data,do only once. */
	if (first_enable) {
		/* register eink irq */
		int i;

		ret = disp_al_init_waveform(param.wavefile_path);
		if (ret) {
			__wrn("malloc and save waveform memory fail!\n");
			disp_al_free_waveform();
		}
		for (i = 0; i < WAVE_DATA_BUF_NUM; i++) {
			ret = disp_al_init_eink_ctrl_data_8(manager->disp,
			    ((unsigned long)(manager->private_data->
			    wavedata_ring_buffer.wavedata_vaddr[i])),
			    &param.timing, 0);
		}
		disp_sys_register_irq(manager->private_data->irq_no, 0,
			      __eink_interrupt_proc, (void *)manager, 0, 0);
		disp_sys_enable_irq(manager->private_data->irq_no);
		manager->detect_fresh_task =
		    kthread_create(eink_detect_fresh_thread, (void *)manager,
				   "eink fresh proc");

		if (IS_ERR_OR_NULL(manager->detect_fresh_task)) {
			__wrn("create eink detect fresh thread fail!\n");
			ret = PTR_ERR(manager->detect_fresh_task);
			return ret;
		}

		ret = wake_up_process(manager->detect_fresh_task);
		first_enable = 0;
	}

	disp_al_eink_irq_enable(manager->disp);

	spin_lock_irqsave(&manager->private_data->slock, flags);

	manager->private_data->enable_flag = true;

	spin_unlock_irqrestore(&manager->private_data->slock, flags);

	return ret;
}

s32 eink_disable(struct disp_eink_manager *manager)
{
	unsigned long flags = 0;
	int ret = 0;

	spin_lock_irqsave(&manager->private_data->slock, flags);

	manager->private_data->enable_flag = false;

	spin_unlock_irqrestore(&manager->private_data->slock, flags);

	ret = disp_al_eink_irq_disable(manager->disp);

	/* disable de. */
	if (manager->convert_mgr)
		ret = manager->convert_mgr->disable(manager->convert_mgr->disp);
	else
		__wrn("convert mgr is null.\n");

	/* disable eink engine. */
	ret = disp_al_eink_disable(manager->disp);

	/* disable clk move to when display finish. */
	/* ret = __eink_clk_disable(manager); */

	return ret;
}

int eink_set_temperature(struct disp_eink_manager *manager, unsigned int temp)
{
	s32 ret = -1;

	if (manager) {
		manager->eink_panel_temperature = temp;
		ret = 0;
	} else {
		__wrn("eink manager is null\n");
	}

	return ret;
}

unsigned int eink_get_temperature(struct disp_eink_manager *manager)
{
	s32 temp = 28;

	if (manager)
		temp = manager->eink_panel_temperature;
	else
		__wrn("eink manager is null\n");

	return temp;
}

s32 eink_resume(struct disp_eink_manager *manager)
{
	int ret = 0;

	if (!suspend)
		return -EBUSY;

	mutex_lock(&eink_manager->standby_lock);
	suspend = 0;
	mutex_unlock(&eink_manager->standby_lock);
	/* register eink irq */
	if (!first_enable) {
		__eink_clk_enable(manager);
		disp_sys_register_irq(manager->private_data->irq_no, 0,
			      __eink_interrupt_proc, (void *)manager, 0, 0);
		disp_sys_enable_irq(manager->private_data->irq_no);
		if (!manager->detect_fresh_task)
			manager->detect_fresh_task =
			    kthread_create(eink_detect_fresh_thread,
					   (void *)manager, "eink fresh proc");

		if (IS_ERR_OR_NULL(manager->detect_fresh_task)) {
			__wrn("create eink detect fresh thread fail!\n");
			ret = PTR_ERR(manager->detect_fresh_task);
			return ret;
		}

		ret = wake_up_process(manager->detect_fresh_task);
	}

	return ret;
}

s32 eink_suspend(struct disp_eink_manager *manager)
{
	int ret = 0;

	if (suspend) {
		__wrn("err, already suspend.\n");
		return -EBUSY;
	}

	mutex_lock(&eink_manager->standby_lock);
	suspend = 1;
	mutex_unlock(&eink_manager->standby_lock);

	if (!first_enable) {
		if (manager->detect_fresh_task) {
			kthread_stop(manager->detect_fresh_task);
			manager->detect_fresh_task = NULL;
		}
	}

	while (!display_finish_flag
	       || !manager->buffer_mgr->is_empty(manager->buffer_mgr)) {
		/* msleep(2); */
		usleep_range(500, 3000);
	}

	if (!first_enable) {
		__eink_clk_disable(manager);
		disp_sys_unregister_irq(manager->private_data->irq_no,
					__eink_interrupt_proc, (void *)manager);
		disp_sys_disable_irq(manager->private_data->irq_no);
	}

	return ret;
}

static int __write_edma(struct disp_eink_manager *manager)
{
	int ret = 0, cur_line = 0, start_delay = 0;
	unsigned long wavedata_paddr = 0;

	current_frame++;
	wavedata_paddr =
	    (unsigned long)__request_buffer_for_display(&manager->private_data->
							wavedata_ring_buffer);
	if (!wavedata_paddr) {
		__wrn("%d frame no wavedata!\n", current_frame);
		return -EBUSY;
	}

	start_delay = disp_al_lcd_get_start_delay(manager->disp, NULL);
	ret = disp_al_eink_edma_cfg_addr(manager->disp, wavedata_paddr);

	cur_line = disp_al_lcd_get_cur_line(manager->disp, NULL);
	if (cur_line < start_delay)
		__wrn("cfg edma too quicker.\n");
	ret = disp_al_dbuf_rdy();

	ret =
	    __dequeue_wavedata_buffer(&manager->private_data->
				      wavedata_ring_buffer);

	return ret;
}

static int __write_edma_first(struct disp_eink_manager *manager)
{
	int ret = 0;
	unsigned long wavedata_paddr = 0;
	struct eink_private *priv = manager->private_data;

	current_frame++;
	wavedata_paddr =
	    (unsigned long)__request_buffer_for_display(&priv->wavedata_ring_buffer);
	if (!wavedata_paddr) {
		__wrn("first frame no wavedata!\n");
		return -EBUSY;
	}

	ret =
	    disp_al_edma_config(manager->disp, wavedata_paddr,
				&priv->param);
	ret = disp_al_edma_write(manager->disp, 1);
	ret = disp_al_dbuf_rdy();

	ret =
	    __dequeue_wavedata_buffer(&priv->wavedata_ring_buffer);

	return ret;
}

int __write_edma_second(struct disp_eink_manager *manager)
{
	int ret = 0, cur_line = 0, start_delay = 0;
	unsigned long wavedata_paddr = 0;
	struct eink_private *priv = manager->private_data;

	cur_line = disp_al_lcd_get_cur_line(manager->disp, 0);
	start_delay = disp_al_lcd_get_start_delay(manager->disp, 0);

	while (cur_line < start_delay)
		cur_line = disp_al_lcd_get_cur_line(manager->disp, 0);

	current_frame++;
	wavedata_paddr =
	    (unsigned long)__request_buffer_for_display(&priv->wavedata_ring_buffer);
	if (!wavedata_paddr) {
		__debug("%d frame no wavedata!\n", current_frame);
		return -EBUSY;
	}
	ret = disp_al_eink_edma_cfg_addr(manager->disp, wavedata_paddr);
	ret = disp_al_dbuf_rdy();
	ret =
	    __dequeue_wavedata_buffer(&priv->wavedata_ring_buffer);

	return ret;
}

/* #define LARGE_MEM_TEST */
#ifdef LARGE_MEM_TEST
static void *malloc_wavedata_buffer(u32 mem_len, void *phy_address)
{
	u32 temp_size = PAGE_ALIGN(mem_len);
	struct page *page = NULL;
	void *tmp_virt_address = NULL;
	unsigned long tmp_phy_address = 0;

	page = alloc_pages(GFP_KERNEL, get_order(temp_size));
	if (page != NULL) {
		tmp_virt_address = page_address(page);
		if (tmp_virt_address == NULL) {
			free_pages((unsigned long)(page), get_order(temp_size));
			__wrn("page_address fail!\n");
			return NULL;
		}

		tmp_phy_address = virt_to_phys(tmp_virt_address);
		*((unsigned long *)phy_address) = tmp_phy_address;

		__inf("pa=0x%p, va=0x%p, size=0x%x, len=0x%x\n",
		      (void *)tmp_phy_address, tmp_virt_address,
		      mem_len, temp_size);
	}

	return tmp_virt_address;
}

static void free_wavedata_buffer(void *virt_addr, void *phys_addr,
				 u32 num_bytes)
{
	unsigned map_size = PAGE_ALIGN(num_bytes);

	free_pages((unsigned long)virt_addr, get_order(map_size));
	virt_addr = NULL;
	phys_addr = NULL;
}
#endif				/* LARGE_MEM_TEST */

int disp_init_eink(struct disp_bsp_init_para *para)
{
	int ret = 0;
	int i = 0;
	unsigned int disp;
	unsigned long image_buf_size;
	unsigned int wavedata_buf_size, hsync, vsync;
	unsigned long indexdata_buf_size = 0;
	struct eink_init_param eink_param;
	struct disp_eink_manager *manager = NULL;
	struct eink_private *data = NULL;
	int wd_buf_id = 0;

	/*
	 * request eink manager and its private data,
	 * then initial this two structure
	 */

	eink_manager =
	    (struct disp_eink_manager *)disp_sys_malloc(sizeof(
						struct disp_eink_manager) *
						MAX_EINK_ENGINE);
	if (eink_manager == NULL) {
		__wrn("malloc eink manager memory fail!\n");
		ret = -ENOMEM;
		goto manager_malloc_err;
	}
	memset(eink_manager, 0,
	       sizeof(struct disp_eink_manager) * MAX_EINK_ENGINE);

	eink_private_data =
		(struct eink_private *)disp_sys_malloc(sizeof
				(struct eink_private)*MAX_EINK_ENGINE);
	if (eink_private_data == NULL) {
		__wrn("malloc private memory fail!\n");
		ret = -ENOMEM;
		goto private_data_malloc_err;
	}
	memset((void *)eink_private_data, 0,
	       sizeof(struct eink_private) * MAX_EINK_ENGINE);

	for (disp = 0; disp < MAX_EINK_ENGINE; disp++) {
		/* get sysconfig and config eink_param */
		__eink_get_sys_config(disp, &eink_param);

		/* load wavefile,and init it. load in enable function now. */
		/*
		 *  ret =  disp_al_init_waveform(eink_param->wavefile_path);
		 *  if (ret) {
		 *  __wrn("malloc and save waveform memory fail!\n");
		 *  goto waveform_err;
		 *  }
		 */
		hsync = eink_param.timing.lbl + eink_param.timing.lel +
						eink_param.timing.lsl;
		vsync =
		    eink_param.timing.fbl + eink_param.timing.fel +
		    eink_param.timing.fsl;
		image_buf_size =
		    eink_param.timing.width * eink_param.timing.height;
		if (eink_param.eink_mode)
			/* mode 1, 16 data */
			wavedata_buf_size = 4 * (eink_param.timing.width / 8 +
				 hsync) * (eink_param.timing.height + vsync);
		else
			/* mode 0, 8 data */
			wavedata_buf_size = 2 * (eink_param.timing.width / 4 +
				 hsync) * (eink_param.timing.height + vsync);

		/* fix it when 5bits */
		if (eink_param.eink_bits < 2)
			/* 3bits or 4bits */
			indexdata_buf_size = image_buf_size;
		else if (eink_param.eink_bits == 2)
			/* 5bits */
			indexdata_buf_size = image_buf_size << 1;

		manager = &eink_manager[disp];
		data = &eink_private_data[disp];

		manager->disp = disp;
		manager->private_data = data;
		manager->mgr = disp_get_layer_manager(disp);
		manager->eink_update = eink_update_image;
		manager->enable = eink_enable;
		manager->disable = eink_disable;
		manager->resume = eink_resume;
		manager->suspend = eink_suspend;
		manager->set_temperature = eink_set_temperature;
		manager->get_temperature = eink_get_temperature;
		manager->eink_panel_temperature = 28;
		manager->op_skip = eink_op_skip;
		/* functions for debug */
#ifdef __EINK_TEST__
		manager->clearwd = __eink_clear_wave_data;
		manager->decode = __eink_debug_decode;
		manager->flush_continue_flag = 1;
#endif
		data->enable_flag = false;
		data->eink_clk = para->mclk[DISP_MOD_EINK];
		data->edma_clk = para->mclk[DISP_MOD_EDMA];
		data->eink_base_addr =
		    (unsigned long)para->reg_base[DISP_MOD_EINK];
		data->irq_no = para->irq_no[DISP_MOD_EINK];
		memcpy((void *)&data->param, (void *)&eink_param,
		       sizeof(struct eink_init_param));

		__debug
		    ("eink_clk: 0x%p; edma_clk: 0x%p; base_addr: 0x%p; irq_no: 0x%x\n",
		     data->eink_clk, data->edma_clk,
		     (void *)data->eink_base_addr, data->irq_no);

		disp_al_set_eink_base(disp, data->eink_base_addr);

#ifdef SUPPORT_WB
		manager->convert_mgr = disp_get_format_manager(disp);
#endif
		spin_lock_init(&data->slock);

		/* init index buffer, it includes old and new index buffer */
		data->index_fresh = false;
		data->new_index = 0;
		data->old_index = 0;
		data->index_vaddr[0] =
		    disp_malloc(indexdata_buf_size * INDEX_BUFFER_NUM,
				(void *)&data->index_paddr[0]);
		if (data->index_vaddr[i] == NULL) {
			__wrn("malloc old index data memory fail!\n");
			ret = -ENOMEM;
			goto private_init_malloc_err;
		}

		memset(data->index_vaddr[0], 0,
		       image_buf_size * INDEX_BUFFER_NUM);
		for (i = 0; i < INDEX_BUFFER_NUM; i++) {
			data->index_paddr[i] =
			    data->index_paddr[0] + indexdata_buf_size * i;
			data->index_vaddr[i] =
			    data->index_vaddr[0] + indexdata_buf_size * i;
			__debug("index_paddr%d:0x%p\n", i,
				data->index_paddr[i]);
		}

		/*
		 * init wavedata ring queue,it includes several wavedata buffer
		 * and queue information.
		 */
		memset(&data->wavedata_ring_buffer, 0,
		       sizeof(struct wavedata_queue));

		spin_lock_init(&data->wavedata_ring_buffer.slock);
		data->wavedata_ring_buffer.head = 0;
		data->wavedata_ring_buffer.tail = 0;
		data->wavedata_ring_buffer.size.width = eink_param.timing.width;
		data->wavedata_ring_buffer.size.height =
		    eink_param.timing.height;
		/* align param need match with drawer's pitch */
		data->wavedata_ring_buffer.size.align = 4;

		/* init wave data,if need lager memory,open the LARGE_MEM_TEST. */
#ifdef LARGE_MEM_TEST
		for (wd_buf_id = 0; wd_buf_id < WAVE_DATA_BUF_NUM; wd_buf_id++) {
			data->wavedata_ring_buffer.wavedata_vaddr[wd_buf_id] =
			    malloc_wavedata_buffer(wavedata_buf_size,
				   &data->wavedata_ring_buffer.wavedata_paddr[wd_buf_id]);
			if (data->wavedata_ring_buffer.
			    wavedata_vaddr[wd_buf_id] == NULL) {
				__wrn
				    ("malloc memory fail, size=%d, id=%d\n",
				     wavedata_buf_size, wd_buf_id);
				ret = -ENOMEM;
				goto private_init_malloc_err;
			}
			memset((void *)data->wavedata_ring_buffer.
			       wavedata_vaddr[wd_buf_id], 0, wavedata_buf_size);

			__debug
			    ("wavedata id=%d, virt-addr=0x%p, phy-addr=0x%p\n",
			     wd_buf_id,
			     (data->wavedata_ring_buffer.wavedata_vaddr[wd_buf_id]),
			     data->wavedata_ring_buffer.wavedata_paddr[wd_buf_id]);
		}
#else
		for (wd_buf_id = 0; wd_buf_id < WAVE_DATA_BUF_NUM; wd_buf_id++) {
			data->wavedata_ring_buffer.wavedata_vaddr[wd_buf_id] =
			    (void *)disp_malloc(wavedata_buf_size,
						&data->wavedata_ring_buffer.
						wavedata_paddr[wd_buf_id]);
			if (data->wavedata_ring_buffer.
			    wavedata_vaddr[wd_buf_id] == NULL) {
				__wrn
				    ("malloc memory fail, size=%d, id=%d\n",
				     wavedata_buf_size, wd_buf_id);
				ret = -ENOMEM;
				goto private_init_malloc_err;
			}
			memset((void *)data->wavedata_ring_buffer.
			       wavedata_vaddr[wd_buf_id], 0, wavedata_buf_size);

			__debug("wavedata id=%d, virt-addr=%p, phy-addr=%p\n",
				wd_buf_id,
				data->wavedata_ring_buffer.wavedata_vaddr[wd_buf_id],
				data->wavedata_ring_buffer.wavedata_paddr[wd_buf_id]);
		}
#endif
		/* init data shell, move it to enable now. */
#if 0
		for (i = 0; i < WAVE_DATA_BUF_NUM; i++) {
			if (eink_param->eink_mode == 0) {
				/* 8 data */
				ret =
				    disp_al_init_eink_ctrl_data_8(manager->disp,
					  (u32)(data->wavedata_ring_buffer.wavedata_vaddr[i])),
					  &eink_param->timing, 0);
			} else {
				/* 16 data */
				ret =
				    disp_al_init_eink_ctrl_data_16(manager->disp,
					   ((u32)(data->wavedata_ring_buffer.wavedata_vaddr[i])),
					   &eink_param->timing);
			}
		}
#endif
		/* register image ring buffer, then fill it to eink manager */
		ring_buffer_manager_init(manager);
		pipeline_manager_init(manager);
		__debug("fresh_idx=%p,total=%p,dec_frame=%p,disp_finish=%d\n",
			&manager->private_data->fresh_frame_index,
			&manager->private_data->total_frame,
			&manager->private_data->decode_frame_index,
			&display_finish_flag);

#ifdef __EINK_TEST__
		u32 total = 0, wave_addr = 0;

		disp_al_get_waveform_data(0, 4, 28, &total, &wave_addr);
		__debug("[disp_al_get_waveform_data]:total=%u wave_addr=0x%x\n",
			total, wave_addr);
#endif
		tasklet_init(&eink_manager->sync_tasklet, __sync_task,
			     (unsigned long)disp);
		INIT_WORK(&eink_manager->decode_work, eink_decode_task);
		mutex_init(&eink_manager->standby_lock);
	}

	return ret;

private_init_malloc_err:
#ifdef LARGE_MEM_TEST
	for (i = 0; i < MAX_EINK_ENGINE; i++) {
		struct wavedata_queue *wd_rbuffer;

		data = &eink_private_data[i];
		wd_rbuffer = &data->wavedata_ring_buffer;
		if (data->index_vaddr[0]) {
			disp_free(data->index_vaddr, data->index_paddr[0],
				  indexdata_buf_size * 2);
			data->index_vaddr[0] = NULL;
		}

		for (wd_buf_id = 0; wd_buf_id < WAVE_DATA_BUF_NUM; wd_buf_id++) {
			if (wd_rbuffer->wavedata_vaddr[wd_buf_id]) {
				free_wavedata_buffer(data->wavedata_ring_buffer.
						     wavedata_vaddr[wd_buf_id],
						     data->wavedata_ring_buffer.
						     wavedata_paddr[wd_buf_id],
						     wavedata_buf_size);
				wd_rbuffer->wavedata_vaddr[wd_buf_id] = NULL;
			}
		}
	}
#else
	for (i = 0; i < MAX_EINK_ENGINE; i++) {
		struct wavedata_queue *wd_rbuffer;

		data = &eink_private_data[i];
		wd_rbuffer = &data->wavedata_ring_buffer;
		if (data->index_vaddr[0]) {
			disp_free(data->index_vaddr, data->index_paddr[0],
				  indexdata_buf_size * 2);
			data->index_vaddr[0] = NULL;
		}

		for (wd_buf_id = 0; wd_buf_id < WAVE_DATA_BUF_NUM; wd_buf_id++) {
			if (wd_rbuffer->wavedata_vaddr[wd_buf_id]) {
				disp_free(wd_rbuffer->wavedata_vaddr[wd_buf_id],
					  wd_rbuffer->wavedata_paddr[wd_buf_id],
					  wavedata_buf_size);
				wd_rbuffer->wavedata_vaddr[wd_buf_id] = NULL;
			}
		}
	}
#endif

/* waveform_err:
 *	disp_al_free_waveform();
 */
private_data_malloc_err:
	kfree(eink_private_data);
	eink_private_data = NULL;

manager_malloc_err:
	kfree(eink_manager);
	eink_manager = NULL;

	return ret;
}

int disp_exit_eink(void)
{
	int i = 0;
	unsigned long image_buf_size;
	unsigned int wavedata_buf_size, hsync, vsync;
	unsigned long indexdata_buf_size = 0;
	struct eink_init_param *eink_param;
	struct disp_eink_manager *manager = NULL;
	struct eink_private *data = NULL;
	int wd_buf_id = 0;

	if (!eink_manager)
		return 0;

	for (i = 0; i < MAX_EINK_ENGINE; i++) {
		struct wavedata_queue *wd_rbuffer;

		manager = &eink_manager[i];
		ring_buffer_manager_exit(manager);
		pipeline_manager_exit(manager);

		data = &eink_private_data[i];
		wd_rbuffer = &data->wavedata_ring_buffer;

		eink_param = &data->param;
		hsync =
		    eink_param->timing.lbl + eink_param->timing.lel +
		    eink_param->timing.lsl;
		vsync =
		    eink_param->timing.fbl + eink_param->timing.fel +
		    eink_param->timing.fsl;
		image_buf_size =
		    eink_param->timing.width * eink_param->timing.height;
		if (eink_param->eink_mode)
			/* mode 1, 16 data */
			wavedata_buf_size = 4 * (eink_param->timing.width / 8 +
				 hsync) * (eink_param->timing.height + vsync);
		else
			/* mode 0, 8 data */
			wavedata_buf_size = 2 * (eink_param->timing.width / 4 +
				 hsync) * (eink_param->timing.height + vsync);
		/* fix it when 5bits */
		if (eink_param->eink_bits < 2)
			/* 3bits or 4bits */
			indexdata_buf_size = image_buf_size;
		else if (eink_param->eink_bits == 2)
			/* 5bits */
			indexdata_buf_size = image_buf_size << 1;

		if (data->index_vaddr[0]) {
			disp_free(data->index_vaddr, data->index_paddr[0],
				  indexdata_buf_size * 2);
			data->index_vaddr[0] = NULL;
		}

		for (wd_buf_id = 0; wd_buf_id < WAVE_DATA_BUF_NUM; wd_buf_id++) {
			if (wd_rbuffer->wavedata_vaddr[wd_buf_id]) {
#ifdef LARGE_MEM_TEST
				free_wavedata_buffer(data->wavedata_ring_buffer.
						     wavedata_vaddr[wd_buf_id],
						     data->wavedata_ring_buffer.
						     wavedata_paddr[wd_buf_id],
						     wavedata_buf_size);
#else
				disp_free(wd_rbuffer->wavedata_vaddr[wd_buf_id],
					  wd_rbuffer->wavedata_paddr[wd_buf_id],
					  wavedata_buf_size);
#endif
				wd_rbuffer->wavedata_vaddr[wd_buf_id] = NULL;
			}
		}
	}

	kfree(eink_private_data);
	kfree(eink_manager);
	eink_private_data = NULL;
	eink_manager = NULL;

	return 0;
}
#endif

