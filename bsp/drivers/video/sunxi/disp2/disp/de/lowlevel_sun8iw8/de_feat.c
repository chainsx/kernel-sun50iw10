/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include "de_feat.h"

static const int de_num_chns[] = {
	/* DISP0 */
	3,
};

static const int de_num_vi_chns[] = {
	/* DISP0 */
	2,
};

static const int de_num_layers[] = {
	/* DISP0 CH0 */
	4,
	/* DISP0 CH1 */
	4,
	/* DISP0 CH2 */
	4,
};

static const int de_is_support_vep[] = {
	/* DISP0 CH0 */
	0,
	/* DISP0 CH1 */
	0,
	/* DISP0 CH2 */
	0,
};

static const int de_is_support_smbl[] = {
	/* CH0 */
	0,
};

static const int de_supported_output_types[] = {
	/* DISP0 */
	1, /* LCD */
};

static const int de_is_support_wb[] = {
	/* DISP0 */
	1,
};

static const int de_is_support_scale[] = {
	/* DISP0 CH0 */
	1,
	/* DISP0 CH1 */
	1,
	/* DISP0 CH2 */
	0,
};

static const int de_scale_line_buffer_yuv[] = {
	/* DISP0 CH0 */
	1024,
	/* DISP0 CH1 */
	1024,
	/* DISP0 CH2 */
	0,
};

static const int de_scale_line_buffer_rgb[] = {
	/* DISP0 CH0 */
	1024,
	/* DISP0 CH1 */
	1024,
	/* DISP0 CH2 */
	0,
};

static const struct de_feat de_cur_features = {
	.num_screens = DE_NUM,
	.num_devices = DEVICE_NUM,
	.num_chns = de_num_chns,
	.num_vi_chns = de_num_vi_chns,
	.num_layers = de_num_layers,
	.is_support_vep = de_is_support_vep,
	.is_support_smbl = de_is_support_smbl,
	.is_support_wb = de_is_support_wb,
	.supported_output_types = de_supported_output_types,
	.is_support_scale = de_is_support_scale,
	.scale_line_buffer_rgb = de_scale_line_buffer_rgb,
	.scale_line_buffer_yuv = de_scale_line_buffer_yuv,
};

int de_feat_get_num_screens(void)
{
	return de_cur_features.num_screens;
}

int de_feat_get_num_devices(void)
{
	return de_cur_features.num_devices;
}

int de_feat_get_num_chns(unsigned int disp)
{
	return de_cur_features.num_chns[disp];
}

int de_feat_get_num_vi_chns(unsigned int disp)
{
	return de_cur_features.num_vi_chns[disp];
}

int de_feat_get_num_ui_chns(unsigned int disp)
{
	return de_cur_features.num_chns[disp] -
	    de_cur_features.num_vi_chns[disp];
}

int de_feat_get_num_layers(unsigned int disp)
{
	unsigned int i, index = 0, num_channels = 0;
	int num_layers = 0;

	if (disp >= de_feat_get_num_devices())
		return 0;

	for (i = 0; i < disp; i++)
		index += de_feat_get_num_chns(i);

	num_channels = de_feat_get_num_chns(disp);
	for (i = 0; i < num_channels; i++, index++)
		num_layers += de_cur_features.num_layers[index];

	return num_layers;
}

int de_feat_get_num_layers_by_chn(unsigned int disp, unsigned int chn)
{
	unsigned int i, index = 0;

	if (disp >= de_feat_get_num_devices())
		return 0;
	if (chn >= de_feat_get_num_chns(disp))
		return 0;

	for (i = 0; i < disp; i++)
		index += de_feat_get_num_chns(i);
	index += chn;

	return de_cur_features.num_layers[index];
}

int de_feat_is_support_vep(unsigned int disp)
{
	unsigned int i, index = 0, num_channels = 0;
	int is_support_vep = 0;

	if (disp >= de_feat_get_num_devices())
		return 0;

	for (i = 0; i < disp; i++)
		index += de_feat_get_num_chns(i);

	num_channels = de_feat_get_num_chns(disp);
	for (i = 0; i < num_channels; i++, index++)
		is_support_vep += de_cur_features.is_support_vep[index];

	return is_support_vep;
}

int de_feat_is_support_vep_by_chn(unsigned int disp, unsigned int chn)
{
	unsigned int i, index = 0;

	if (disp >= de_feat_get_num_devices())
		return 0;
	if (chn >= de_feat_get_num_chns(disp))
		return 0;

	for (i = 0; i < disp; i++)
		index += de_feat_get_num_chns(i);
	index += chn;

	return de_cur_features.is_support_vep[index];
}

int de_feat_is_support_smbl(unsigned int disp)
{
	return de_cur_features.is_support_smbl[disp];
}

int de_feat_is_supported_output_types(unsigned int disp,
				      unsigned int output_type)
{
	return (de_cur_features.supported_output_types[disp] & output_type);
}

int de_feat_is_support_wb(unsigned int disp)
{
	return de_cur_features.is_support_wb[disp];
}

int de_feat_is_support_scale(unsigned int disp)
{
	unsigned int i, index = 0, num_channels = 0;
	int is_support_scale = 0;

	if (disp >= de_feat_get_num_devices())
		return 0;

	for (i = 0; i < disp; i++)
		index += de_feat_get_num_chns(i);

	num_channels = de_feat_get_num_chns(disp);
	for (i = 0; i < num_channels; i++, index++)
		is_support_scale += de_cur_features.is_support_scale[index];

	return is_support_scale;
}

int de_feat_is_support_scale_by_chn(unsigned int disp, unsigned int chn)
{
	unsigned int i, index = 0;

	if (disp >= de_feat_get_num_devices())
		return 0;
	if (chn >= de_feat_get_num_chns(disp))
		return 0;

	for (i = 0; i < disp; i++)
		index += de_feat_get_num_chns(i);
	index += chn;

	return de_cur_features.is_support_scale[index];
}

int de_feat_get_scale_linebuf_for_yuv(unsigned int disp, unsigned int chn)
{
	unsigned int i, index = 0;

	if (disp >= de_feat_get_num_screens())
		return 0;
	if (chn >= de_feat_get_num_chns(disp))
		return 0;

	for (i = 0; i < disp; i++)
		index += de_feat_get_num_chns(i);
	index += chn;

	return de_cur_features.scale_line_buffer_yuv[index];
}

int de_feat_get_scale_linebuf_for_rgb(unsigned int disp, unsigned int chn)
{
	unsigned int i, index = 0;

	if (disp >= de_feat_get_num_screens())
		return 0;
	if (chn >= de_feat_get_num_chns(disp))
		return 0;

	for (i = 0; i < disp; i++)
		index += de_feat_get_num_chns(i);
	index += chn;

	return de_cur_features.scale_line_buffer_rgb[index];
}

/**
 * @name       de_feat_get_number_of_vdpo
 * @brief      get number of vdpo
 * @param[IN]  none
 * @param[OUT] none
 * @return     number of vdpo
 */
unsigned int de_feat_get_number_of_vdpo(void)
{
	/* return de_cur_features->num_vdpo; */
	return 0;
}

int de_feat_init(void)
{
#if 0
	{
		unsigned int num_screens;
		__inf("------------FEAT---------\n");
		num_screens = de_feat_get_num_devices();
		DE_INF("device:%d\n", num_screens);
		for (disp = 0; disp < num_screens; disp++) {
			unsigned int num_chns = de_feat_get_num_chns(disp);
			unsigned int num_layers = de_feat_get_num_layers(disp);
			unsigned int i;
			__inf("device %d: %d chns, %d layers\n", disp, num_chns,
			      num_layers);
			for (i = 0; i < num_chns; i++) {
				num_layers =
				    de_feat_get_num_layers_by_chn(disp, i);
				DE_INF("device %d, chn %d: %d layers\n", disp,
				       i, num_layers);
			}
		}
		__inf("-------------------------\n");
	}
#endif
	return 0;
}

int de_feat_exit(void)
{
	return 0;
}
