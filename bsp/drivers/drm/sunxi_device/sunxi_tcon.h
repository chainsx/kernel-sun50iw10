/*sunxi_tcon.h
 *
 * Copyright (C) 2022 Allwinnertech Co., Ltd.
 * Authors: zhengwanyu <zhengwanyu@allwinnertech.com>
 * Authors: hongyaobin <hongyaobin@allwinnertech.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#ifndef _SUNXI_TCON_H_
#define _SUNXI_TCON_H_

#include <linux/clk.h>
#include "sunxi_config.h"
#include "de/include.h"
#include "sunxi_common.h"

#define TCON_NUM_MAX      4

#if defined(CONFIG_ARCH_SUN8IW12)
#define DISP2_TCON_TV_SYNC_POL_ISSUE
#endif

enum tcon_type {
	TCON_LCD = 0,
	TCON_TV = 1,
};
struct lcd_clk_info;

struct sunxi_tcon_funcs {
	bool (*is_use_irq)(unsigned int enc_id);
	unsigned int (*get_irq_no)(unsigned int enc_id);

	bool (*conn_is_support)(int enc_id, int conn_type_id);
	int (*set)(unsigned int de_id,
		   unsigned int enc_id,
		   unsigned int conn_type_id,
		   struct disp_video_timings *video_info,
		   struct sunxi_connector_work_mode *conn_work_mode);
	int (*sw_set)(unsigned int enc_id, unsigned int conn_type_id);
	void (*unset)(unsigned int enc_id);
};

struct sunxi_tcon {
	/* id(or number) of this tcon */
	int			id;
	bool			is_enabled;

	/* register base */
	uintptr_t		reg_base;

	/* 0:tcon_lcd  1:tcon_tv */
	enum tcon_type		type;

	/*
	 * for example, typde_id = 1, type = TCON_LCD,
	 * it means that this sunxi_tcon is TCON_LCD1
	 */
	int			type_id;

	/* clock resource */
	struct clk 		*mclk;/* module clk */
	struct clk      *mclk_bus;/* module clk bus */
	unsigned long long	tcon_div;

	/* interrupt resource */
	unsigned int 		irq_no;
	/* judge_line for start delay, used to judge if there is enough time
	 *to update and sync DE register
	 */
	unsigned int	judge_line;

	/* indicate that this tcon has been designed to a connector
	 *and can NOT be assigned to others
	 */
	bool 			is_assigned;

	/* Attached connector type id */
	int				conn_type_id;

	struct sunxi_tcon_funcs *funcs;
	struct reset_control *rst_bus_tcon;
};

#if defined(HAVE_DEVICE_COMMON_MODULE)
/* disp_if_top module */
struct sunxi_top_if {
	uintptr_t               reg_base;
	struct clk              *mclk;/* module clk */
	struct reset_control 	*rst_bus_if_top;
};
#endif

struct sunxi_tcon_drv {
	struct platform_device *pdev;
	struct drv_model_info  drv_model;

#if defined(HAVE_DEVICE_COMMON_MODULE)
	struct sunxi_top_if hwtopif;
#endif
	/* total counts of tcon */
	unsigned int tcon_cnt;
	unsigned int tcon_lcd_cnt;
	unsigned int tcon_tv_cnt;
	struct sunxi_tcon hwtcon[TCON_NUM_MAX];
};

int sunxi_tcon_lcd_enable(int nr, int lcd_id);
int sunxi_tcon_lcd_disable(int nr, int lcd_id);

unsigned int sunxi_tcon_mode(unsigned int id);
unsigned int sunxi_tcon_slave_num(unsigned int id);
unsigned int sunxi_tcon_port_num(unsigned int id);


struct sunxi_tcon_funcs *
sunxi_tcon_attach_connector_type(int enc_id, int conn_type);
void sunxi_tcon_unattach_connector_type(int enc_id);


enum disp_tv_output sunxi_tv_get_interface_type(int tv_id);
int sunxi_tcon_query_irq(int nr);
int sunxi_tcon_lcd_get_tcon_id(unsigned int lcd_id);
bool sunxi_tcon_sync_time_is_enough(unsigned int nr);
int sunxi_tcon_get_count(void);

int sunxi_tcon_module_init(void);
void sunxi_tcon_module_exit(void);
int sunxi_tcon_get_clk_info(int lcd_id, struct lcd_clk_info *info);

#endif
