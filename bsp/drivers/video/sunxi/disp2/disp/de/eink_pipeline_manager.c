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

#define PIPELINE_NUM 16
static int op_skip;
static struct mutex mlock;

static int __overlap_judge_skip(struct pipeline_manager *manager, u32 skip)
{
	mutex_lock(&mlock);
	op_skip = skip;
	mutex_unlock(&mlock);
	return 0;
}

/* return 0, no overlap, 1 overlap. */
static int __overlap_judge(struct area_info pipeline_area,
			   struct area_info update_area)
{
	struct area_info *left_area;
	struct area_info *right_area;

	struct area_info *top_area;
	struct area_info *bottom_area;

	mutex_lock(&mlock);
	if (op_skip) {
		mutex_unlock(&mlock);
		return 0;
	}
	mutex_unlock(&mlock);

	if (update_area.x_top > pipeline_area.x_top) {
		right_area = &update_area;
		left_area = &pipeline_area;
	} else {
		right_area = &pipeline_area;
		left_area = &update_area;
	}

	/* x not overlap, two area could not be overlay,
	 * if equal,is also not overlap.
	 */
	if (right_area->x_top >= left_area->x_bottom)
		return 0;

	if (update_area.y_top > pipeline_area.y_top) {
		top_area = &update_area;
		bottom_area = &pipeline_area;
	} else {
		top_area = &pipeline_area;
		bottom_area = &update_area;
	}

	/* y not overlap, two area could not be overlay */
	if (top_area->y_top >= bottom_area->y_bottom)
		return 0;

	__debug("xtop=%u ,ytop=%u ,x_bo=%u ,y_bo=%u\n",
		update_area.x_top, update_area.y_top,
		update_area.x_bottom, update_area.y_bottom);

	return 1;

}

/* return 0: empty, return 1: full */
int __free_list_status(struct pipeline_manager *manager)
{
	struct pipeline_info *pipeline, *tpipeline;
	int num = 0;
	int ret = 0;

	mutex_lock(&manager->list.mlock);
	if (list_empty(&manager->list.used_list)) {
		ret = 0;
		goto out;
	}

	list_for_each_entry_safe(pipeline, tpipeline, &manager->list.free_list,
				 node) {
		num++;
	}
	if (num == PIPELINE_NUM) {
		ret = 1;
		goto out;
	} else if (num > PIPELINE_NUM) {
		ret = -1;
	}

out:
	mutex_unlock(&manager->list.mlock);

	return ret;
}

/* return 0: empty, return 1: full,
 * return 2: not full,not empty,
 * return -1: err
*/
int __used_list_status(struct pipeline_manager *manager)
{
	struct pipeline_info *pipeline, *tpipeline;
	int ret = 0;
	int used_num = 0;

	mutex_lock(&manager->list.mlock);

	if (list_empty(&manager->list.used_list)) {
		ret = 0;
		goto out;
	}

	list_for_each_entry_safe(pipeline, tpipeline,
				 &manager->list.used_list, node)
		used_num++;

	if (used_num == PIPELINE_NUM)
		ret = 1;
	else if (used_num > PIPELINE_NUM)
		ret = -1;
	else
		ret = 2;

out:
	mutex_unlock(&manager->list.mlock);

	return ret;
}

int __check_overlap(struct pipeline_manager *manager, struct area_info area)
{
	int ret = 0;
	struct pipeline_info *pipeline, *tpipeline;

	mutex_lock(&manager->list.mlock);

	list_for_each_entry_safe(pipeline, tpipeline, &manager->list.used_list,
				 node) {
		if (__overlap_judge(pipeline->area, area)) {
			pipeline->overlap_flag = 1;
			mutex_lock(&manager->mlock);
			manager->overlap_num++;
			mutex_unlock(&manager->mlock);
		}
	}

	mutex_unlock(&manager->list.mlock);

	return ret;
}

int __check_overlap_num(struct pipeline_manager *manager)
{
	int ret;

	mutex_lock(&manager->mlock);

	if (manager->overlap_num != 0)
		ret = 1;
	else
		ret = 0;

	mutex_unlock(&manager->mlock);

	return ret;
}

int __clear_pipeline_list(struct pipeline_manager *manager)
{
	int ret = 0;
	struct pipeline_info *pipeline, *tpipeline;

	mutex_lock(&manager->mlock);

	list_for_each_entry_safe(pipeline, tpipeline,
				 &manager->list.used_list, node) {

		list_move_tail(&pipeline->node, &manager->list.free_list);

		pipeline->overlap_flag = 0;
		pipeline->frame_index = 0;
		pipeline->total_frames = 0;
		pipeline->enable_flag = 0;

	}
	manager->overlap_num = 0;

	mutex_unlock(&manager->mlock);
	return ret;
}

/* return 0:no new pipeline insert
 * return 1:one new pipeline insert
 * return -1:more than one pipeline insert,something is wrong.
*/
int __update_pipeline_list(struct pipeline_manager *manager,
			   unsigned int temperature, unsigned int *tframes)
{
	int ret = 0;
	struct pipeline_info *pipeline, *tpipeline;
	unsigned int cur_wave_paddr;
	unsigned int new_pipelin_num = 0;

	mutex_lock(&manager->list.mlock);
	list_for_each_entry_safe(pipeline, tpipeline, &manager->list.used_list,
				 node) {
		if ((pipeline->frame_index == pipeline->total_frames)
		    && (pipeline->total_frames != 0)) {
			disp_al_eink_pipe_disable(manager->disp,
						  pipeline->pipeline_no);
			pipeline->frame_index = 0;
			pipeline->total_frames = 0;
			pipeline->enable_flag = 0;

			list_move_tail(&pipeline->node,
				       &manager->list.free_list);

			mutex_lock(&manager->mlock);

			if (pipeline->overlap_flag)
				manager->overlap_num--;
			if (manager->overlap_num < 0)
				__wrn("overlap_num is wrong.");

			pipeline->overlap_flag = 0;

			mutex_unlock(&manager->mlock);
			continue;
		}

		if (pipeline->frame_index < 1) {
			ret =
			    disp_al_get_eink_panel_bit_num(manager->disp,
							   &pipeline->bit_num);
			ret =
			    disp_al_get_waveform_data(manager->disp,
						      pipeline->mode,
						      temperature,
						      &pipeline->total_frames,
						      &pipeline->
						      wave_file_addr);
		}
		cur_wave_paddr =
		    pipeline->wave_file_addr +
		    (1 << (pipeline->bit_num << 1)) * pipeline->frame_index;

		disp_al_eink_pipe_config_wavefile(manager->disp, cur_wave_paddr,
						  pipeline->pipeline_no);

		pipeline->frame_index++;
		if (!pipeline->enable_flag) {
			disp_al_eink_pipe_enable(manager->disp,
						 pipeline->pipeline_no);
			pipeline->enable_flag = true;
			new_pipelin_num++;
			*tframes = pipeline->total_frames;
			ret = 1;
		}
		if (new_pipelin_num > 1) {
			__wrn
			    ("more than one new pipelines have been inserted!");
			ret = -1;
		}
	}

	mutex_unlock(&manager->list.mlock);

	return ret;
}

int __config_and_enable_one_pipeline(struct pipeline_manager *manager,
				     struct area_info update_area,
				     enum eink_update_mode mode,
				     unsigned int temperature,
				     unsigned int *tframes)
{
	struct pipeline_info *pipeline, *tpipeline;
	int ret = 0;
	unsigned int cur_wave_paddr;

	mutex_lock(&manager->list.mlock);

	list_for_each_entry_safe(pipeline, tpipeline, &manager->list.free_list,
				 node) {
		pipeline->overlap_flag = 0;
		pipeline->frame_index = 0;
		pipeline->mode = mode;
		memcpy((void *)&pipeline->area, (void *)&update_area,
		       (unsigned int)sizeof(struct area_info));

		if (pipeline->enable_flag)
			__wrn("free pipeline list have enable pipeline!");

		pipeline->enable_flag = true;

		ret =
		    disp_al_get_eink_panel_bit_num(manager->disp,
						   &pipeline->bit_num);

		ret = disp_al_get_waveform_data(manager->disp, pipeline->mode,
						temperature,
						&pipeline->total_frames,
						&pipeline->wave_file_addr);
		*tframes = pipeline->total_frames;
		cur_wave_paddr = pipeline->wave_file_addr
		    + (1 << (pipeline->bit_num << 1)) * pipeline->frame_index;

		ret =
		    disp_al_eink_pipe_config(manager->disp,
					     pipeline->pipeline_no,
					     pipeline->area);
		ret =
		    disp_al_eink_pipe_config_wavefile(manager->disp,
						      cur_wave_paddr,
						      pipeline->pipeline_no);
		ret =
		    disp_al_eink_pipe_enable(manager->disp,
					     pipeline->pipeline_no);

		pipeline->frame_index++;

		list_move_tail(&pipeline->node, &manager->list.used_list);

		break;
	}

	mutex_unlock(&manager->list.mlock);

	return ret;
}

int __config_one_pipeline(struct pipeline_manager *manager,
			  struct area_info update_area,
			  enum eink_update_mode mode)
{
	struct pipeline_info *pipeline, *tpipeline;
	int ret = 0;

	mutex_lock(&manager->list.mlock);

	list_for_each_entry_safe(pipeline, tpipeline, &manager->list.free_list,
				 node) {
		pipeline->overlap_flag = 0;
		pipeline->frame_index = 0;
		pipeline->mode = mode;

		if (pipeline->enable_flag) {
			pipeline->enable_flag = false;
			__wrn("free pipeline list have used pipeline!");
		}

		memcpy((void *)&pipeline->area, (void *)&update_area,
		       sizeof(struct area_info));

		/*
		 * no need config wave file addr,
		 * it will config when next decode task.
		 */
		disp_al_eink_pipe_config(manager->disp, pipeline->pipeline_no,
					 pipeline->area);
		list_move_tail(&pipeline->node, &manager->list.used_list);
		break;
	}

	mutex_unlock(&manager->list.mlock);

	return ret;
}

int pipeline_manager_init(struct disp_eink_manager *eink_manager)
{
	int ret = 0;
	int i;
	struct pipeline_manager *pipeline_manager;
	struct pipeline_info *pipeline[PIPELINE_NUM];

	pipeline_manager =
	   (struct pipeline_manager *)disp_sys_malloc(
				sizeof(struct pipeline_manager));
	if (!pipeline_manager) {
		__wrn("malloc eink pipeline manager memory fail!\n");
		ret = -ENOMEM;
		goto pipeline_mgr_err;
	}
	memset((void *)pipeline_manager, 0, sizeof(struct pipeline_manager));

	pipeline_manager->disp = eink_manager->disp;
	pipeline_manager->overlap_num = 0;

	INIT_LIST_HEAD(&pipeline_manager->list.free_list);
	INIT_LIST_HEAD(&pipeline_manager->list.used_list);

	mutex_init(&pipeline_manager->mlock);
	mutex_init(&pipeline_manager->list.mlock);
	mutex_init(&mlock);

	for (i = 0; i < PIPELINE_NUM; i++) {
		pipeline[i] =
		   (struct pipeline_info *)disp_sys_malloc(
						sizeof(struct pipeline_info));
		if (!pipeline[i]) {
			__wrn("malloc eink pipeline memory fail!\n");
			ret = -ENOMEM;
			goto pipeline_err;
		}
		memset((void *)pipeline[i], 0, sizeof(struct pipeline_info));
		pipeline[i]->pipeline_no = i;
		list_add_tail(&pipeline[i]->node,
			      &pipeline_manager->list.free_list);
	}

	pipeline_manager->check_overlap = __check_overlap;
	pipeline_manager->op_skip = __overlap_judge_skip;
	pipeline_manager->config_one_pipeline = __config_one_pipeline;
	pipeline_manager->config_and_enable_one_pipeline =
	    __config_and_enable_one_pipeline;
	pipeline_manager->update_pipeline_list = __update_pipeline_list;
	pipeline_manager->used_list_status = __used_list_status;
	pipeline_manager->free_list_status = __free_list_status;
	pipeline_manager->check_overlap_num = __check_overlap_num;
	pipeline_manager->clear_pipeline_list = __clear_pipeline_list;
	eink_manager->pipeline_mgr = pipeline_manager;

	return ret;

pipeline_err:
	for (i = 0; i < PIPELINE_NUM; i++)
		kfree(pipeline[i]);

pipeline_mgr_err:
	kfree(pipeline_manager);

	return ret;
}

int pipeline_manager_exit(struct disp_eink_manager *eink_manager)
{
	int i = 0;
	struct pipeline_manager *pipeline_manager;
	struct pipeline_info *pipeline = NULL, *pipelinetmp = NULL;

	pipeline_manager = eink_manager->pipeline_mgr;
	list_add_tail(&pipeline[i].node,
		      &pipeline_manager->list.free_list);
	list_for_each_entry_safe(pipeline, pipelinetmp,
				 &pipeline_manager->list.free_list,
				 node) {
		list_del(&pipeline->node);
		kfree((void *)pipeline);
	}

	kfree(pipeline_manager);
	eink_manager->pipeline_mgr = NULL;

	return 0;
}
#endif

