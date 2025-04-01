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
#ifndef _SUNXI_DE_H_
#define _SUNXI_DE_H_
#include <linux/clk.h>
#include "sunxi_config.h"
#include "de/include.h"
#include "sunxi_common.h"

#define DE_NUM_MAX	4
#define DE_CHN_MAX  4
#define DE_LAYER_MAX  4

struct sunxi_de_funcs {
	bool (*is_support_tcon)(int nr, int tcon_id);
	unsigned long (*get_freq)(int nr);

	int (*get_layer_count)(int nr);
	int (*get_vi_layer_count)(int nr);
	int (*get_ui_layer_count)(int nr);
	int (*get_layer_channel_id)(int nr, int layer_id);
	int (*get_layer_id_within_chanel)(int nr, int top_layer_id);
	int (*get_layer_formats)(int nr, unsigned int layer_id,
					 const unsigned int **formats,
					 unsigned int *count);
	int (*single_layer_apply)(int nr, struct disp_layer_config_data *data);
	int (*multi_layers_apply)(int nr, struct disp_layer_config_data *data,
					unsigned int layer_num);
	bool (*is_enable)(int nr);
	int (*enable)(int nr, struct disp_manager_data *data);
	void (*disable)(int nr, struct disp_manager_data *data);

	bool (*is_use_irq)(int nr);
	unsigned int (*get_irq_no)(int nr);
	int (*query_irq)(int nr);
	int (*event_proc)(int nr, bool update);

	/* enhance */
	int (*set_enhance)(int nr, int mode, bool enable,
			int width, int height, int conn_type);
	void (*get_enhance)(int nr,
			int *mode, bool *enable, int *width, int *height);

	/* smbl */
	bool (*is_support_smbl)(int nr);
	int (*set_smbl)(int nr, bool enable,
			struct disp_rect *rect);
	void (*get_smbl)(int nr, bool *enable,
			struct disp_rect *rect);

};

struct sunxi_de {
	/* id(or number) of this de */
	int			id;

	/* channel count
	* vichannel: video channel, support yuv/rgb inputs
	* uichannel: UI channel, only support rgb inputs
	*/
	unsigned int		vichannel_cnt;
	unsigned int		uichannel_cnt;

	/* layers per channel */
	/* NOTE: this two members designed below is NOT perfect,
	* We assume that the layers counts of all vi channel are same, so are
	* the ui channel. it is order to reduce the complexity of codes.
	* But if the layers of channels in DE HW is different from each other,
	* there will CAUSE BUG!!!!!!
	*/
	unsigned int		layers_per_vichannel;
	unsigned int		layers_per_uichannel;

	bool layer_enabled[DE_CHN_MAX][DE_LAYER_MAX];

	/* interrupt resource */
	bool 			irq_enable;
	bool 			irq_used;
	unsigned int 		irq_no;
	bool use_rcq;

	/* indicate that this de has been designed to a tcon
	 * and can NOT be assigned to others
	 */
	bool			enable;
	/* show how this de working, this structrue is referred from
	 * sunxi_display
	 */
	struct disp_manager_info	info;

	struct disp_layer_config_data *layer_config_data;

	/* enhance */
	struct disp_enhance_config enhance_config;
	struct disp_smbl_info smbl_info;

	const struct sunxi_de_funcs *funcs;
};

/*
 * NOTE: The hardware de0 and de1 is designed to one hardware module,
 * so they have same clk resources
 */
struct sunxi_de_drv {
	struct platform_device *pdev;
	struct drv_model_info  drv_model;

	/* the counts of de that this platform device has */
	unsigned char		de_cnt;

	/* register base, the lowlevel sw layer of de will differ the
	 * reg_base of de0 and de1, so we just need to provide one reg_base
	 */
	uintptr_t		reg_base;

	/* clock resource */
	struct clk 		*mclk;/* module clk */
	struct clk      *mclk_bus;/* module bus clk */

	unsigned int irq_no;
#if defined(CONFIG_AW_DRM_DE_V33X)
	/* only used int de_v33x */
	unsigned int chn_cfg_mode;
#endif
	struct reset_control *rst_bus_de;
	struct sunxi_de hwde[DE_NUM_MAX];
};

int sunxi_de_get_count(void);
struct sunxi_de_funcs *sunxi_de_get_funcs(int nr);

int sunxi_de_module_init(void);
void sunxi_de_module_exit(void);
#endif
