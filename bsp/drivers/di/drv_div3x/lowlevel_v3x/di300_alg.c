/*
 * Copyright (c) 2007-2018 Allwinnertech Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include "di300_alg.h"
#include "../di_debug.h"

u32 p2_period[8] = { PROD22, PROD32, PROD2332, PROD2224, PROD32322,
	PROD55, PROD64, PROD87
};

/* p2 lowhigh */
u8 p2_lh_22[PROD22] = { 1, 1 };
u8 p2_lh_32[PROD32] = { 1, 1, 1, 0, 1 };
u8 p2_lh_2332[PROD2332] = { 1, 1, 1, 0, 1, 1, 0, 1, 1, 1 };
u8 p2_lh_2224[PROD2224] = { 1, 1, 1, 1, 1, 1, 1, 0, 0, 1 };
u8 p2_lh_32322[PROD32322] = { 1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1 };
u8 p2_lh_55[PROD55] = { 1, 0, 0, 0, 1 };
u8 p2_lh_64[PROD64] = { 1, 0, 0, 0, 0, 1, 1, 0, 0, 1 };
u8 p2_lh_87[PROD87] = { 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1 };

/* p1 lowhigh */
u8 p1_lh_22[PROD22] = { 0, 1 };
u8 p1_lh_32[PROD32] = { 0, 1, 0, 0, 1 };
u8 p1_lh_2332[PROD2332] = { 0, 1, 0, 0, 1, 0, 0, 1, 0, 1 };
u8 p1_lh_2224[PROD2224] = { 0, 1, 0, 1, 0, 1, 0, 0, 0, 1 };
u8 p1_lh_32322[PROD32322] = { 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1 };
u8 p1_lh_55[PROD55] = { 0, 0, 0, 0, 1 };
u8 p1_lh_64[PROD64] = { 0, 0, 0, 0, 0, 1, 0, 0, 0, 1 };
u8 p1_lh_87[PROD87] = { 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1 };

/* field weave phase */
u8 fw_ph_22[PROD22] = { 0, 2 };
u8 fw_ph_32[PROD32] = { 2, 0, 2, 1, 0 };
u8 fw_ph_2332[PROD2332] = { 2, 0, 2, 1, 0, 2, 1, 0, 2, 0 };
u8 fw_ph_2224[PROD2224] = { 2, 0, 2, 0, 2, 0, 2, 1, 1, 0 };
u8 fw_ph_32322[PROD32322] = { 2, 1, 0, 2, 0, 2, 1, 0, 2, 0, 2, 0 };
u8 fw_ph_55[PROD55] = { 2, 1, 1, 1, 0 };
u8 fw_ph_64[PROD64] = { 2, 1, 1, 1, 1, 0, 2, 1, 1, 0 };
u8 fw_ph_87[PROD87] = { 2, 1, 1, 1, 1, 1, 1, 0, 2, 1, 1, 1, 1, 1, 0 };

/*******************************************************************/
/*                  Field Order Detection Algorithm                */
/*******************************************************************/
static void di_alg_field_order_detection(
	struct __fmd_hist *fmd_hist,
	struct __di_para_t *di_para,
	struct __fod_alg_hist *alg_hist)
{
	s32 mf0diff, mf1diff, intraf0diff, intraf1diff;
	struct __fod_alg_para *alg_para = &di_para->alg_para.fod_alg_para;
	s32 maxmfdiff;
	s32 diffs;

	/* f0t and f1b */
	mf0diff = (di_para->bff) ? fmd_hist->FOD_FID12 : fmd_hist->FOD_FID30;
	/* f0b and f1t */
	mf1diff = (di_para->bff) ? fmd_hist->FOD_FID30 : fmd_hist->FOD_FID12;

	intraf0diff = fmd_hist->FOD_FID10;	/* f0t and f0b */
	intraf1diff = fmd_hist->FOD_FID32;	/* f1t and f1b */

	maxmfdiff = mf0diff >= mf1diff ? mf0diff : mf1diff;
	/* one of intra-frame difference not small than
	 * mean(or max) of inter-frame difference
	 */
	if (intraf0diff * alg_para->R_kick > maxmfdiff ||
	    intraf1diff * alg_para->R_kick > maxmfdiff) {
		diffs = mf0diff - mf1diff;

		DI_INFO("large than maxmfdiff, intraf0diff:%d intraf1diff:%d alg_para->R_kick:%d\n",
				intraf0diff, intraf1diff, alg_para->R_kick);
		DI_INFO("mf0diff:%d mf1diff:%d\n", mf0diff, mf1diff);
		DI_INFO("di_para->bff:%d\n", di_para->bff);
		if (di_para->bff == 0) {
			if ((diffs < 0) &&
			    ((mf0diff * alg_para->R_rev_10) < (mf1diff * 10)) &&
			    (mf1diff > alg_para->T_rev)) {
				DI_INFO("bff 0, diffs:%d\n", diffs);
				DI_INFO("R_rev_10 compare:%d %d\n",
					mf0diff * alg_para->R_rev_10, mf1diff * 10);
				DI_INFO("mf1diff:%d T_rev:%d\n", mf1diff, alg_para->T_rev);
				alg_hist->rev_cnt_bff += 1;
			} else {
				alg_hist->rev_cnt_bff = 0;
				alg_hist->rev_cnt_tff = 0;
			}
		} else {
			if ((diffs > 0) &&
			    ((mf1diff * alg_para->R_rev_10) < (mf0diff * 10)) &&
			    (mf0diff > alg_para->T_rev)) {
				DI_INFO("bff 1, diffs:%d\n", diffs);
				DI_INFO("R_rev_10 compare:%d %d\n",
					mf1diff * alg_para->R_rev_10, mf0diff * 10);
				DI_INFO("mf1diff:%d T_rev:%d\n", mf0diff, alg_para->T_rev);
				alg_hist->rev_cnt_tff += 1;
			} else {
				alg_hist->rev_cnt_bff = 0;
				alg_hist->rev_cnt_tff = 0;
			}
		}

		DI_INFO("rev_cnt_bff:%d rev_cnt_tff:%d T_rev_time:%d\n",
				alg_hist->rev_cnt_bff, alg_hist->rev_cnt_tff, alg_para->T_rev_time);
		if (alg_hist->rev_cnt_bff == alg_para->T_rev_time) {
			alg_hist->bff_fix = 1;
			alg_hist->rev_cnt_bff = 0;
			alg_hist->rev_cnt_tff = 0;
			alg_hist->is_fieldorderchange = 1;
		} else if (alg_hist->rev_cnt_tff == alg_para->T_rev_time) {
			alg_hist->bff_fix = 0;
			alg_hist->rev_cnt_bff = 0;
			alg_hist->rev_cnt_tff = 0;
			alg_hist->is_fieldorderchange = 1;
		} else {
			alg_hist->bff_fix = di_para->bff;
			alg_hist->is_fieldorderchange = 0;
		}
	} else {
		DI_INFO("lower than maxmfdiff\n");
		alg_hist->bff_fix = di_para->bff;
		alg_hist->is_fieldorderchange = 0;
	}
}

/*******************************************************************/
/*                    Film Mode Detection Algorithm                */
/*******************************************************************/
static u32 di_alg_fmd_sc_detect(u32 sc_array[], u32 *sc_length,
	u32 cur_totalfid, u32 r_sc,
	u32 t_p1diff, u32 t_p1diff_u, u32 cadence_cur)
{
	u32 is_scenechange;
	u32 i;
	u32 sum_sc, mean_sc;

	is_scenechange = 0;
	if (*sc_length == 4) {
		sum_sc = 0;
		for (i = 0; i < *sc_length; i++)
			sum_sc += sc_array[i];
		mean_sc = sum_sc / 4;

		if ((cur_totalfid > mean_sc * r_sc &&
		     cur_totalfid > t_p1diff) || (cur_totalfid > t_p1diff_u))
			is_scenechange = 1;
	} else {
		if (cur_totalfid > t_p1diff_u)
			is_scenechange = 1;
	}

	/* update sc_array */
	if (cadence_cur) {
		if (*sc_length < 4 && *sc_length > 0) {
			(*sc_length)++;
			sc_array[(*sc_length) - 1] = cur_totalfid;
		} else {
			for (i = 0; i < 3; i++)
				sc_array[i] = sc_array[i + 1];
			sc_array[3] = cur_totalfid;
		}
	}

	return is_scenechange;
}

static u32 di_alg_fmd_22_trace(u32 weave_phase_pre,
	u32 *weave_phase_cur, u32 cadence_cur,
	u32 is_scenechange, u32 is_scenetrace)
{
	u32 predict_lowhigh;
	u32 is_lock;

	/* update weave_phase */
	*weave_phase_cur = (1 - weave_phase_pre);

	/* update predict_lowhigh */
	predict_lowhigh = (*weave_phase_cur) ? 0 : 1;

	/* trace */
	is_lock = 1;

	/* Lost lock situation
	 * 1.normal cadence break situation
	 * 2.scene change break situation
	 */
	if (predict_lowhigh == 0 && cadence_cur == 1)
		is_lock = 0;
	else if ((is_scenechange && predict_lowhigh == 0) ||
		(is_scenetrace && predict_lowhigh == 1 && cadence_cur == 0))
		is_lock = 0;

	return is_lock;
}

static u32 di_alg_fmd_22_detect(
	u32 cadence_cur[], u32 init_field_cnt,
	u32 *weave_phase, s32 period22_x0,
	s32 period22_10, u32 fmd_22_en)
{
	u32 is_lock;
	u32 ped = 2;
	s32 i;
	s32 log_10, log_x0;

	is_lock = 0;
	log_10 = 0;
	log_x0 = 0;

	if ((!init_field_cnt) || (init_field_cnt > FMD22FIELDNUM)) {
		DI_DEBUG("WARN: %s init_field_cnt is too big\n", __func__);
		DI_DEBUG("WARN: init_field_cnt:%d  FMD22FIELDNUM:%d\n", init_field_cnt,
								FMD22FIELDNUM);
		return is_lock;
	}

	if (init_field_cnt > 0 && cadence_cur[init_field_cnt - 1] == 0 &&
	    init_field_cnt >= period22_x0 * ped && fmd_22_en == 1) {
		for (i = 1; i <= period22_x0; i++) {
			if (cadence_cur[init_field_cnt - i * ped] == 1 &&
			    cadence_cur[init_field_cnt - i * ped + 1] == 0)
				log_10++;
			if (cadence_cur[init_field_cnt - i * ped + 1] == 0)
				log_x0++;
		}

		if (log_x0 == period22_x0 && log_10 >= period22_10) {
			is_lock = 1;
			*weave_phase = 1;
		}
	}
	return is_lock;
}

static void di_alg_fmd_p1(u32 cur_fid, u32 pre_fid, u32 t_thrl,
	u32 t_thrh, u32 avgns, u32 r_film22rel, u32 r_film22rel2,
	u32 cad_low_array[], u32 *cad_low_length, u32 *cadence_cur)
{
	u32 high_fid, low_fid;
	u32 highcan;
	s32 i;
	u32 sum_cad_low, mean_cad_low, cad_low_valid;

	high_fid = cur_fid >= pre_fid ? cur_fid : pre_fid;
	low_fid = cur_fid <= pre_fid ? cur_fid : pre_fid;

	highcan = (cur_fid > pre_fid) ? 1 : 0;

	if (*cad_low_length < 10) {
		cad_low_valid = 0;
		mean_cad_low = 0;
	} else {
		cad_low_valid = 1;
		sum_cad_low = 0;
		for (i = 0; i < 10; i++)
			sum_cad_low += cad_low_array[i];
		mean_cad_low = sum_cad_low / 10;
	}

	/* decide cadence_cur */
	if (((highcan == 1) &&
		(low_fid < t_thrl) &&
		((low_fid + avgns) * r_film22rel < (high_fid + avgns))) ||
		(cur_fid > t_thrh) ||
		(low_fid > t_thrl) ||
		(((cur_fid + avgns) > r_film22rel2 * (mean_cad_low + avgns)) &&
		(cad_low_valid == 1))) {
		*cadence_cur = 1;
	} else {
		*cadence_cur = 0;
	}

	/* update cad_low_array */
	if (*cadence_cur == 0) {
		if (*cad_low_length < 10) {
			cad_low_array[*cad_low_length] = cur_fid;
			*cad_low_length = *cad_low_length + 1;
		} else {
			for (i = 0; i < 9; i++)
				cad_low_array[i] = cad_low_array[i + 1];
			cad_low_array[9] = cur_fid;
		}
	}
}

static void di_alg_fmd_22(struct __fmd_hist *fmd_hist,
	struct __di_para_t *di_para,
	struct __fmd_alg_hist *alg_hist,
	u32 is_fo_change, u32 is_text_field, u32 base_field)
{
	u32 p1_init_phase = alg_hist->p1_init_phase;
	u32 init_field_cnt_22 = alg_hist->init_field_cnt_22;
	u32 is_22_lock_pre = alg_hist->is_22_lock;
	s32 sc_trace_cnt = alg_hist->sc_trace_cnt;
	u32 sc_length = alg_hist->sc_length;
	u32 pre_totalfid = alg_hist->pre_totalfid;
	u32 cur_totalfid = (base_field) ? fmd_hist->FMD_FID23 :
	    fmd_hist->FMD_FID12;
	u32 cad_low_length = alg_hist->cad_low_length;
	u32 weave_phase_22_pre = alg_hist->weave_phase_22;
	u32 weave_phase_22_cur = 0;
	u32 is_22_lock_cur;
	u32 is_scenechange;
	u32 is_scenetrace;
	u32 is_22_temp_di;
	u32 cadence_cur;

	s32 i, width, heightf;
	u32 t_p1diffthrl, t_p1diffthrh, avgns;
	u32 t_p1diff, t_p1diff_u;

	width = di_para->width;
	heightf = di_para->height / 2;

	/* if no pre field fid, pass the detection */
	if (alg_hist->pre_totalfid_valid == 0)
		goto ALG_FMD_22_END;

	/* Calculate cadence_cur */
	t_p1diffthrl = di_para->alg_para.fmd_alg_para.t_p1diffthrl_pixel *
	    width * heightf;
	t_p1diffthrh = di_para->alg_para.fmd_alg_para.t_p1diffthrh_pixel *
	    width * heightf;
	avgns = di_para->alg_para.fmd_alg_para.avgns_pixel * width * heightf;

	di_alg_fmd_p1(cur_totalfid, pre_totalfid, t_p1diffthrl, t_p1diffthrh,
		avgns,
		di_para->alg_para.fmd_alg_para.r_film22rel,
		di_para->alg_para.fmd_alg_para.r_film22rel2,
		alg_hist->cad_low_array, &cad_low_length, &cadence_cur);

	/* Update cadence_cur */
	if (init_field_cnt_22 < FMD22FIELDNUM) {
		alg_hist->cadence_cur[init_field_cnt_22] = cadence_cur;
	} else {
		for (i = 0; i < FMD22FIELDNUM - 1; i++)
			alg_hist->cadence_cur[i] = alg_hist->cadence_cur[i + 1];
		alg_hist->cadence_cur[FMD22FIELDNUM - 1] = cadence_cur;
	}

	p1_init_phase = (init_field_cnt_22 < 1) ? 0 : 1;
	if (init_field_cnt_22 < FMD22FIELDNUM)
		init_field_cnt_22++;

	t_p1diff = di_para->alg_para.fmd_alg_para.t_p1diff_pixel *
	    width * heightf;
	t_p1diff_u = di_para->alg_para.fmd_alg_para.t_p1diff_u_pixel *
	    width * heightf;
	is_scenechange = di_alg_fmd_sc_detect(alg_hist->sc_array, &sc_length,
		cur_totalfid, di_para->alg_para.fmd_alg_para.r_sc,
		t_p1diff, t_p1diff_u, cadence_cur);

	if (is_scenechange)
		init_field_cnt_22 = 0;

	/* trace or detect */
	is_scenetrace = (is_22_lock_pre && sc_trace_cnt > 0) ? 1 : 0;
	if (is_22_lock_pre) {	/* if last field 2-2 lock */
		is_22_lock_cur = di_alg_fmd_22_trace(
			weave_phase_22_pre, &weave_phase_22_cur,
			cadence_cur, is_scenechange, is_scenetrace);

		if (is_text_field)	/* when text exist, all film mode unlock */
			is_22_lock_cur = 0;

		if (is_22_lock_cur) {
			if (is_scenechange)
				sc_trace_cnt =
				    di_para->alg_para.fmd_alg_para.sc_trace_fnum;

			if (is_scenetrace)
				sc_trace_cnt -= 1;
		} else {
			init_field_cnt_22 = 0;
			sc_trace_cnt = 0;
			cad_low_length = 0;
		}
	} else {
		if (p1_init_phase == 1 && is_text_field == 0)
			is_22_lock_cur =
				di_alg_fmd_22_detect(alg_hist->cadence_cur,
					init_field_cnt_22, &weave_phase_22_cur,
					di_para->alg_para.fmd_alg_para.period22_x0,
					di_para->alg_para.fmd_alg_para.period22_10,
					di_para->alg_para.fmd_alg_para.fmd_22_en);
		else
			is_22_lock_cur = 0;
	}

	/* is_22_temp_di */
	if (is_22_lock_cur
		&& (is_scenechange || is_scenetrace))
		is_22_temp_di = 1;
	else
		is_22_temp_di = 0;

	/* update hist */
	alg_hist->p1_init_phase = p1_init_phase;
	alg_hist->init_field_cnt_22 = init_field_cnt_22;
	alg_hist->is_22_lock = is_22_lock_cur;
	alg_hist->is_22_temp_di = is_22_temp_di;
	alg_hist->sc_trace_cnt = sc_trace_cnt;
	alg_hist->sc_length = sc_length;
	alg_hist->cad_low_length = cad_low_length;
	alg_hist->weave_phase_22 = weave_phase_22_cur;

	if (base_field)
		alg_hist->is_scenechange_f4 = is_scenechange;
	else
		alg_hist->is_scenechange_f3 = is_scenechange;
	if (base_field)
		alg_hist->is_scenechange_f4 = is_scenechange;
	else
		alg_hist->is_scenechange_f3 = is_scenechange;

ALG_FMD_22_END:
	alg_hist->pre_totalfid = cur_totalfid;
	alg_hist->pre_totalfid_valid = 1;
}

static void di_alg_fmd_init_lh(
	u32 p2_array[15], u32 *high_p2diff, u32 *low_p2diff)
{
	s32 i, j;
	u32 max_seq[9];
	u32 min_maxseq;
	u32 sum_seq5678, mean5678;

	/* max_seq[0] */
	max_seq[0] = p2_array[0];
	*low_p2diff = p2_array[0];
	for (j = 1; j < 6; j++) {
		if (max_seq[0] < p2_array[j])
			max_seq[0] = p2_array[j];

		if (*low_p2diff > p2_array[j])
			*low_p2diff = p2_array[j];
	}

	/* max_seq[1] ~ max_seq[8] */
	for (i = 1; i < 9; i++) {
		if (p2_array[i + 5] > max_seq[i - 1])
			max_seq[i] = p2_array[i + 5];
		else
			max_seq[i] = max_seq[i - 1];

		if (*low_p2diff > p2_array[i + 5])
			*low_p2diff = p2_array[i + 5];
	}

	min_maxseq = max_seq[1];
	for (i = 2; i < 9; i++) {
		if (min_maxseq > max_seq[i])
			min_maxseq = max_seq[i];
	}

	sum_seq5678 = 0;
	for (i = 5; i < 9; i++) {
		sum_seq5678 += max_seq[i];
	}
	mean5678 = sum_seq5678 / 4;

	*high_p2diff = (mean5678 + min_maxseq) / 2;

}

static s32 di_alg_fmd_calc_meanlowbefore(
	u32 p2diff_la[], u32 la_length, u32 low_p2diff)
{
	s32 meanlowbefore;
	u32 i;
	u32 tmp_sum;

	tmp_sum = 0;
	switch (la_length) {
	case 0:
		meanlowbefore = low_p2diff;
		break;
	case 1:
		meanlowbefore = p2diff_la[0];
		break;
	case 2:
		meanlowbefore = (p2diff_la[0] + p2diff_la[1]) / 2;
		break;
	case 3:
		meanlowbefore = (p2diff_la[1] + p2diff_la[2]) / 2;
		break;
	case 4:
	case 5:
	case 6:
	case 7:
		for (i = la_length - 4; i < la_length; i++)
			tmp_sum += p2diff_la[i];
		meanlowbefore = tmp_sum / 4;
		break;
	case 8:
	case 9:
	case 10:
	case 11:
	case 12:
	case 13:
	case 14:
	case 15:
		for (i = la_length - 8; i < la_length; i++)
			tmp_sum += p2diff_la[i];
		meanlowbefore = tmp_sum / 8;
		break;
	default:
		for (i = 0; i < la_length; i++)
			tmp_sum += p2diff_la[i];
		meanlowbefore = tmp_sum / 16;
		break;

	}
	return meanlowbefore;
}

static s32 di_alg_fmd_calc_biaslowbefore(
	u32 p2diff_la[], u32 la_length, s32 meanlowbefore)
{
	s32 biaslowbefore;
	u32 i;
	u32 tmp_sum;

	tmp_sum = 0;
	switch (la_length) {
	case 0:
	case 1:
	case 2:
	case 3:
		biaslowbefore = 0;
		break;
	case 4:
	case 5:
	case 6:
	case 7:
		for (i = la_length - 4; i < la_length; i++)
			tmp_sum +=
			    abs((s32) p2diff_la[i] - (s32) meanlowbefore);
		biaslowbefore = tmp_sum / 4;
		break;
	case 8:
	case 9:
	case 10:
	case 11:
	case 12:
	case 13:
	case 14:
	case 15:
		for (i = la_length - 8; i < la_length; i++)
			tmp_sum +=
			    abs((s32) p2diff_la[i] - (s32) meanlowbefore);
		biaslowbefore = tmp_sum / 8;
		break;
	default:
		for (i = 0; i < la_length; i++)
			tmp_sum +=
			    abs((s32) p2diff_la[i] - (s32) meanlowbefore);
		biaslowbefore = tmp_sum / 16;
		break;
	}
	return biaslowbefore;
}

static s32 di_alg_fmd_calc_meanhighbefore(
	u32 p2diff_ha[], u32 ha_length, u32 high_p2diff)
{
	s32 meanhighbefore;
	u32 i;
	u32 tmp_sum;

	tmp_sum = 0;
	switch (ha_length) {
	case 0:
		meanhighbefore = high_p2diff;
		break;
	case 1:
		meanhighbefore = p2diff_ha[0];
		break;
	case 2:
		meanhighbefore = (p2diff_ha[0] + p2diff_ha[1]) / 2;
		break;
	case 3:
		meanhighbefore = (p2diff_ha[1] + p2diff_ha[2]) / 2;
		break;
	case 4:
	case 5:
	case 6:
	case 7:
		for (i = ha_length - 4; i < ha_length; i++)
			tmp_sum += p2diff_ha[i];
		meanhighbefore = tmp_sum / 4;
		break;
	case 8:
	case 9:
	case 10:
	case 11:
	case 12:
	case 13:
	case 14:
	case 15:
		for (i = ha_length - 8; i < ha_length; i++)
			tmp_sum += p2diff_ha[i];
		meanhighbefore = tmp_sum / 8;
		break;
	default:
		for (i = 0; i < ha_length; i++)
			tmp_sum += p2diff_ha[i];
		meanhighbefore = tmp_sum / 16;
		break;

	}
	return meanhighbefore;
}

static void di_alg_fmd_calc_classifier_var(
	u32 p2diff_la[], u32 p2diff_ha[],
	u32 la_length, u32 ha_length,
	u32 high_p2diff, u32 low_p2diff,
	s32 *meanlowbefore, s32 *biaslowbefore, s32 *meanhighbefore)
{
	*meanlowbefore = di_alg_fmd_calc_meanlowbefore(
		p2diff_la, la_length, low_p2diff);
	*biaslowbefore = di_alg_fmd_calc_biaslowbefore(
		p2diff_la, la_length, *meanlowbefore);
	*meanhighbefore = di_alg_fmd_calc_meanhighbefore(
		p2diff_ha, ha_length, high_p2diff);
}

static u32 di_alg_fmd_calc_lowhigh(s32 tmp_value, s32 meanlowbefore,
	s32 biaslowbefore, s32 meanhighbefore,
	u32 la_length, u32 high_p2diff,
	s32 hl_ratio, s32 r_sigma3, s32 t_mlb,
	s32 t_value, u32 r_lowcad, s32 t_sigma3, u32 r_sc)
{
	u32 low_high;
	s32 sigma3;

	low_high = 0;

	/* classifier */
	if ((meanhighbefore - tmp_value) <=
	    (tmp_value - meanlowbefore) * (s32) r_lowcad) {
		if (la_length < 4) {
			if ((tmp_value - meanlowbefore) * 16 >
			    hl_ratio * (meanhighbefore - meanlowbefore))
				low_high = 1;
		} else {
			sigma3 = 3 * biaslowbefore * r_sigma3;

			if (meanlowbefore < t_mlb) {
				/* hl_ratio = hl_ratio; */
			} else if (meanlowbefore < t_mlb * 3 / 2)
				hl_ratio = hl_ratio - 1;
			else if (meanlowbefore < 2 * t_mlb)
				hl_ratio = hl_ratio - 2;
			else
				hl_ratio = hl_ratio - 3;

			if ((tmp_value > t_value) ||
				(tmp_value > (meanlowbefore + sigma3)))
				low_high = 1;
			else if ((sigma3 > t_sigma3) ||
				(sigma3 * 4 < meanlowbefore)) {
				if ((tmp_value - meanlowbefore) * 16 >
					hl_ratio * (meanhighbefore - meanlowbefore))
					low_high = 1;
			}
		}
	}
	return low_high;
}

static void di_alg_fmd_feedback(u32 p2diff_la[], u32 p2diff_ha[],
	s32 *la_length, s32 *ha_length,
	u32 low_high, u32 tmp_value,
	u32 meanhighbefore, u32 r_fb_sc,
	u32 low_index[], u32 high_index[], u32 field_index)
{
	u32 i;
	if (low_high) {
		if (tmp_value > meanhighbefore * r_fb_sc)
			tmp_value = meanhighbefore;

		if (*ha_length == 16) {
			for (i = 0; i < 15; i++) {
				p2diff_ha[i] = p2diff_ha[i + 1];
				high_index[i] = high_index[i + 1];
			}
		} else {
			*ha_length = *ha_length + 1;
		}

		if (*ha_length > 0) {
			p2diff_ha[*ha_length - 1] = tmp_value;
			high_index[*ha_length - 1] = field_index;
		}
	} else {
		if (*la_length == 16) {
			for (i = 0; i < 15; i++) {
				p2diff_la[i] = p2diff_la[i + 1];
				low_index[i] = low_index[i + 1];
			}
		} else {
			*la_length = *la_length + 1;
		}

		if (*la_length > 0) {
			p2diff_la[*la_length - 1] = tmp_value;
			low_index[*la_length - 1] = field_index;
		}
	}
}

static u32 di_alg_fmd_lowhigh_classifier(u32 p2_array_cur,
	u32 p2diff_la[], u32 p2diff_ha[],
	s32 *la_length, s32 *ha_length,
	u32 high_p2diff, u32 low_p2diff,
	u32 *low_index, u32 *high_index, s32 hl_ratio,
	s32 r_sigma3, s32 t_mlb, s32 t_value,
	u32 r_lowcad, s32 t_sigma3,
	u32 r_sc, u32 r_fb_sc, u32 field_index)
{
	u32 low_high;
	s32 tmp_value;
	s32 meanlowbefore, biaslowbefore, meanhighbefore;

	if (p2_array_cur > high_p2diff * r_sc)
		tmp_value = high_p2diff;
	else
		tmp_value = p2_array_cur;

	di_alg_fmd_calc_classifier_var(p2diff_la, p2diff_ha,
		*la_length, *ha_length, high_p2diff, low_p2diff,
		&meanlowbefore, &biaslowbefore, &meanhighbefore);
	low_high = di_alg_fmd_calc_lowhigh(tmp_value, meanlowbefore,
		biaslowbefore, meanhighbefore,
		*la_length, high_p2diff,
		hl_ratio, r_sigma3, t_mlb, t_value,
		r_lowcad, t_sigma3, r_sc);
	di_alg_fmd_feedback(p2diff_la, p2diff_ha,
		la_length, ha_length, low_high, tmp_value,
		meanhighbefore, r_fb_sc, low_index,
		high_index, field_index);

	return low_high;
}

static u32 di_alg_fmd_non22_phase(u32 phase, u32 film_mode)
{
	u32 new_phase;
	u32 period;

	period = p2_period[film_mode];
	new_phase = phase + 1;

	return new_phase = new_phase % period;
}

static u32 di_alg_fmd_non22_p2_trace(
	u32 phase, u32 film_mode, u32 lowhigh,
	u32 cadence_cur, u32 cur_totalfid,
	u32 fid_trace_en, u32 t_p1diff_trace)
{
	u32 is_lock;
	u32 predict_lowhigh;
	u8 *p2_lh, *p1_lh;

	is_lock = 1;

	switch (film_mode) {
	case 1:
		p2_lh = p2_lh_32;
		break;
	case 2:
		p2_lh = p2_lh_2332;
		break;
	case 3:
		p2_lh = p2_lh_2224;
		break;
	case 4:
		p2_lh = p2_lh_32322;
		break;
	case 5:
		p2_lh = p2_lh_55;
		break;
	case 6:
		p2_lh = p2_lh_64;
		break;
	case 7:
		p2_lh = p2_lh_87;
		break;
	default:
		p2_lh = p2_lh_22;
		break;
	}

	predict_lowhigh = p2_lh[phase];

	/* use p2diff to trace */
	if (predict_lowhigh == 0 && lowhigh == 1)
		is_lock = 0;

	/* use p1diff to trace */
	if (fid_trace_en) {
		switch (film_mode) {
		case 1:
			p1_lh = p1_lh_32;
			break;
		case 2:
			p1_lh = p1_lh_2332;
			break;
		case 3:
			p1_lh = p1_lh_2224;
			break;
		case 4:
			p1_lh = p1_lh_32322;
			break;
		case 5:
			p1_lh = p1_lh_55;
			break;
		case 6:
			p1_lh = p1_lh_64;
			break;
		case 7:
			p1_lh = p1_lh_87;
			break;
		default:
			p1_lh = p1_lh_22;
			break;
		}

		predict_lowhigh = p1_lh[phase];

		if (predict_lowhigh == 0 && cadence_cur == 1 &&
		    cur_totalfid >= t_p1diff_trace)
			is_lock = 0;
	}

	return is_lock;
}

static u32 di_alg_fmd_non22_p1_trace(
	u32 phase, u32 film_mode, u32 cadence_cur)
{
	u32 is_lock;
	u8 *p1_lh;
	u32 predict_lowhigh;

	is_lock = 1;

	switch (film_mode) {
	case 1:
		p1_lh = p1_lh_32;
		break;
	case 2:
		p1_lh = p1_lh_2332;
		break;
	case 3:
		p1_lh = p1_lh_2224;
		break;
	case 4:
		p1_lh = p1_lh_32322;
		break;
	case 5:
		p1_lh = p1_lh_55;
		break;
	case 6:
		p1_lh = p1_lh_64;
		break;
	case 7:
		p1_lh = p1_lh_87;
		break;
	default:
		p1_lh = p1_lh_22;
		break;
	}

	predict_lowhigh = p1_lh[phase];

	if (predict_lowhigh == 0 && cadence_cur == 1)
		is_lock = 0;

	return is_lock;
}

static u32 di_alg_fmd_index_detect_ped1(
	u32 pat, s32 enterperiod,
	s32 index_length, u32 index_diff[])
{
	u32 log_ok;
	s32 i;
	u32 is_lock;

	log_ok = 0;
	is_lock = 0;
	if (index_length >= enterperiod) {
		for (i = 1; i <= enterperiod; i++) {
			if (index_diff[index_length - i] == pat)
				log_ok++;
			else {
				is_lock = 0;
				goto LOCK_DETECT_FAIL;
			}
		}
		is_lock = 1;
	}
LOCK_DETECT_FAIL:
	return is_lock;
}

static u32 di_alg_fmd_index_detect_ped2(s32 ped, u32 pat2[],
	s32 enterperiod, s32 index_length, u32 index_diff[])
{
	u32 log_ok;
	s32 i;
	u32 is_lock;

	log_ok = 0;
	is_lock = 0;
	if (index_length >= enterperiod * ped) {
		for (i = 1; i <= enterperiod; i++) {
			if (index_diff[index_length - i * ped] == pat2[0] &&
			    index_diff[index_length - i * ped + 1] == pat2[1])
				log_ok++;
			else {
				is_lock = 0;
				goto LOCK_DETECT_FAIL;
			}
		}
		is_lock = 1;
	}
LOCK_DETECT_FAIL:
	return is_lock;
}

static u32 di_alg_fmd_index_detect_ped4(s32 ped, u32 pat4[],
	s32 enterperiod, s32 index_length, u32 index_diff[])
{
	u32 log_ok;
	s32 i;
	u32 is_lock;

	log_ok = 0;
	is_lock = 0;
	if (index_length >= enterperiod * ped) {
		for (i = 1; i <= enterperiod; i++) {
			if (index_diff[index_length - i * ped] == pat4[0] &&
			    index_diff[index_length - i * ped + 1] == pat4[1] &&
			    index_diff[index_length - i * ped + 2] == pat4[2] &&
			    index_diff[index_length - i * ped + 3] == pat4[3])
				log_ok++;
			else {
				is_lock = 0;
				goto LOCK_DETECT_FAIL;
			}
		}
		is_lock = 1;
	}
LOCK_DETECT_FAIL:
	return is_lock;
}

static u32 di_alg_fmd_non22_detect(
	u32 low_index[], u32 high_index[],
	s32 la_length, s32 ha_length, u32 lowhigh[],
	u32 *mode, u32 *phase,
	u32 fmd_32_en, s32 period32,
	u32 fmd_2224_en, s32 period2224,
	u32 fmd_2332_en, s32 period2332,
	u32 fmd_32322_en, s32 period32322,
	u32 fmd_55_en, s32 period55,
	u32 fmd_64_en, s32 period64,
	u32 fmd_87_en, s32 period87)
{
	u32 is_lock;
	u32 low_index_diff[15], high_index_diff[15];
	s32 i;
	u32 recycle;
	u32 sum_lowhigh;
	u32 usehighidx;
	s32 ped;

	is_lock = 0;

	recycle = 0;
	if (la_length > 2) {
		for (i = 0; i < la_length - 1; i++) {
			if (low_index[i] > low_index[i + 1]) {
				/* recycle */
				low_index_diff[i] = 0xf0000000 - low_index[i] +
				    low_index[i + 1];
				recycle++;
			} else {
				low_index_diff[i] = low_index[i + 1] -
				    low_index[i];
			}
		}
	}

	/* more than 1 time recycle, it must be a non-film mode video */
	if (recycle > 1)
		return is_lock;

	recycle = 0;
	if (ha_length > 2) {
		for (i = 0; i < ha_length - 1; i++) {
			if (high_index[i] > high_index[i + 1]) {
				/* recycle */
				high_index_diff[i] = 0xf0000000 -
				    high_index[i] + high_index[i + 1];
				recycle++;
			} else {
				high_index_diff[i] = high_index[i + 1] -
				    high_index[i];
			}
		}
	}

	/* more than 1 time recycle, it must be a non-film mode video */
	if (recycle > 1)
		return is_lock;

	sum_lowhigh = 0;
	for (i = 0; i < FMD32FIELDNUM; i++)
		sum_lowhigh += lowhigh[i];

	usehighidx = (sum_lowhigh > FMD32FIELDNUM / 2) ? 0 : 1;

	if (usehighidx) {
		if (fmd_55_en) {
			/* phase5 */
			u32 pat2[2][2] = { {1, 4}, {4, 1} };
			ped = 2;
			is_lock = di_alg_fmd_index_detect_ped2(ped, pat2[0],
				period55, ha_length - 1, high_index_diff);
			if (is_lock) {
				*mode = FM_55;
				*phase = 4;
				return is_lock;
			}

			/* phase1 */
			is_lock = di_alg_fmd_index_detect_ped2(ped, pat2[1],
				period55, ha_length - 1, high_index_diff);
			if (is_lock) {
				*mode = FM_55;
				*phase = 0;
				return is_lock;
			}
		}

		if (fmd_64_en) {
			/* phase10 */
			u32 pat4[4][4] = {
				{1, 5, 1, 3},
				{5, 1, 3, 1},
				{1, 3, 1, 5},
				{3, 1, 5, 1}
			};
			ped = 4;
			is_lock = di_alg_fmd_index_detect_ped4(ped, pat4[0],
				period64, ha_length - 1, high_index_diff);
			if (is_lock) {
				*mode = FM_64;
				*phase = 9;
				return is_lock;
			}

			/* phase1 */
			is_lock = di_alg_fmd_index_detect_ped4(ped, pat4[1],
				period64, ha_length - 1, high_index_diff);
			if (is_lock) {
				*mode = FM_64;
				*phase = 0;
				return is_lock;
			}

			/* phase6 */
			is_lock = di_alg_fmd_index_detect_ped4(ped, pat4[2],
				period64, ha_length - 1, high_index_diff);
			if (is_lock) {
				*mode = FM_64;
				*phase = 5;
				return is_lock;
			}

			/* phase7 */
			is_lock = di_alg_fmd_index_detect_ped4(ped, pat4[3],
				period64, ha_length - 1, high_index_diff);
			if (is_lock) {
				*mode = FM_64;
				*phase = 6;
				return is_lock;
			}
		}

		if (fmd_87_en) {
			/* phase15 */
			u32 pat4[4][4] = {
				{1, 7, 1, 6},
				{7, 1, 6, 1},
				{1, 6, 1, 7},
				{6, 1, 7, 1}
			};
			ped = 4;
			is_lock = di_alg_fmd_index_detect_ped4(ped, pat4[0],
				period87, ha_length - 1, high_index_diff);
			if (is_lock) {
				*mode = FM_87;
				*phase = 14;
				return is_lock;
			}

			/* phase1 */
			is_lock = di_alg_fmd_index_detect_ped4(ped, pat4[1],
				period87, ha_length - 1, high_index_diff);
			if (is_lock) {
				*mode = FM_87;
				*phase = 0;
				return is_lock;
			}

			/* phase8 */
			is_lock = di_alg_fmd_index_detect_ped4(ped, pat4[2],
				period87, ha_length - 1, high_index_diff);
			if (is_lock) {
				*mode = FM_87;
				*phase = 7;
				return is_lock;
			}

			/* phase9 */
			is_lock = di_alg_fmd_index_detect_ped4(ped, pat4[3],
				period87, ha_length - 1, high_index_diff);
			if (is_lock) {
				*mode = FM_87;
				*phase = 8;
				return is_lock;
			}
		}
	} else {
		if (fmd_32_en) {
			/* phase4 */
			u32 pat1 = 5;
			ped = 1;
			is_lock = di_alg_fmd_index_detect_ped1(pat1,
				period32, la_length - 1, low_index_diff);
			if (is_lock) {
				*mode = FM_32;
				*phase = 3;
				return is_lock;
			}
		}

		if (fmd_2332_en) {
			/* phase4 */
			u32 pat2[2][2] = { {3, 7}, {7, 3} };
			ped = 2;
			is_lock = di_alg_fmd_index_detect_ped2(ped, pat2[0],
				period2332, la_length - 1, low_index_diff);
			if (is_lock) {
				*mode = FM_2332;
				*phase = 3;
				return is_lock;
			}

			/* phase7 */
			is_lock = di_alg_fmd_index_detect_ped2(ped, pat2[1],
				period2332, la_length - 1, low_index_diff);
			if (is_lock) {
				*mode = FM_2332;
				*phase = 6;
				return is_lock;
			}
		}

		if (fmd_2224_en) {
			/* phase8 */
			u32 pat2[2][2] = { {1, 9}, {9, 1} };
			ped = 2;
			is_lock = di_alg_fmd_index_detect_ped2(ped, pat2[0],
				period2224, la_length - 1, low_index_diff);
			if (is_lock) {
				*mode = FM_2224;
				*phase = 7;
				return is_lock;
			}

			/* phase9 */
			is_lock = di_alg_fmd_index_detect_ped2(ped, pat2[1],
				period2224, la_length - 1, low_index_diff);
			if (is_lock) {
				*mode = FM_2224;
				*phase = 8;
				return is_lock;
			}
		}

		if (fmd_32322_en) {
			/* phase2 */
			u32 pat2[2][2] = { {5, 7}, {7, 5} };
			ped = 2;
			is_lock = di_alg_fmd_index_detect_ped2(ped, pat2[0],
				period32322, la_length - 1, low_index_diff);
			if (is_lock) {
				*mode = FM_32322;
				*phase = 1;
				return is_lock;
			}

			/* phase7 */
			is_lock = di_alg_fmd_index_detect_ped2(ped, pat2[1],
				period32322, la_length - 1, low_index_diff);
			if (is_lock) {
				*mode = FM_32322;
				*phase = 6;
				return is_lock;
			}
		}
	}
	return is_lock;
}

static void di_alg_fmd_non22(struct __fmd_hist *fmd_hist,
	struct __di_para_t *di_para,
	struct __fmd_alg_hist *alg_hist,
	u32 is_fo_change, u32 is_text_field, u32 base_field)
{
	s32 init_field_cnt = alg_hist->init_field_cnt_non22;
	u32 is_scenechange = base_field ?
	    alg_hist->is_scenechange_f4 : alg_hist->is_scenechange_f3;
	u32 is_lock_pre = alg_hist->is_non22_lock;
	u32 is_lock_cur = 0;
	s32 la_length = alg_hist->la_length;
	s32 ha_length = alg_hist->ha_length;
	u32 *p2diff_la = alg_hist->p2diff_la;
	u32 *p2diff_ha = alg_hist->p2diff_ha;
	u32 *low_index = alg_hist->low_index;
	u32 *high_index = alg_hist->high_index;
	u32 *p2_array = alg_hist->p2_array;
	u32 *lowhigh = alg_hist->lowhigh;
	u32 cur_totalfrd = (base_field) ? fmd_hist->FMD_FRD13 :
	    fmd_hist->FMD_FRD02;
	u32 cur_totalfid = (base_field) ? fmd_hist->FMD_FID23 :
	    fmd_hist->FMD_FID12;
	u32 weave_phase_pre = alg_hist->weave_phase_non22;
	u32 film_mode_non22 = alg_hist->film_mode_non22;
	u32 *cadence_cur = alg_hist->cadence_cur;
	u32 init_field_cnt_22 = alg_hist->init_field_cnt_22;
	u32 field_index = alg_hist->field_index;
	u32 is_non22_temp_di;
	u32 weave_phase_cur = 0;
	u32 cad_low_length = alg_hist->cad_low_length;
	s32 i, width, heightf, init_cnt;
	s32 not_enough_for_p2, first_time_p2_ok, p2_init_phase;
	u32 high_p2diff, low_p2diff;

	DI_DEBUG("base_field:%d\ncur_totalfrd:0x%x\ncur_totalfid:0x%x\n",
		base_field, cur_totalfrd, cur_totalfid);

	width = di_para->width;
	heightf = di_para->height / 2;

	if (field_index == 0xf0000000)
		field_index = 0;
	else
		field_index++;

	if (is_scenechange) {
		init_field_cnt = -2;
		la_length = 0;
		ha_length = 0;
	}

	if (init_field_cnt < (FMD32FIELDNUM - 1)) {
		not_enough_for_p2 = 1;
		first_time_p2_ok = 0;
		p2_init_phase = 0;
	} else if (init_field_cnt == (FMD32FIELDNUM - 1)) {
		not_enough_for_p2 = 0;
		first_time_p2_ok = 1;
		p2_init_phase = 1;
	} else {
		not_enough_for_p2 = 0;
		first_time_p2_ok = 0;
		p2_init_phase = 1;
	}

	if (init_field_cnt < FMD32FIELDNUM)
		init_field_cnt++;

	if (init_field_cnt > 0) {
		if (first_time_p2_ok | not_enough_for_p2)
			p2_array[init_field_cnt - 1] = cur_totalfrd;
		else {
			for (i = 0; i < FMD32FIELDNUM - 1; i++)
				p2_array[i] = p2_array[i + 1];
			p2_array[FMD32FIELDNUM - 1] = cur_totalfrd;
		}
	}

	/* classifier low_high */
	if (not_enough_for_p2 == 0) {
		di_alg_fmd_init_lh(p2_array, &high_p2diff, &low_p2diff);
		if (first_time_p2_ok) {
			init_cnt = 0;
		} else if (not_enough_for_p2 == 0) {
			init_cnt = FMD32FIELDNUM - 1;
			/* update lowhigh */
			for (i = 0; i < FMD32FIELDNUM - 1; i++)
				lowhigh[i] = lowhigh[i + 1];
		}
		for (i = init_cnt; i < FMD32FIELDNUM; i++) {
			lowhigh[i] = di_alg_fmd_lowhigh_classifier(p2_array[i],
				p2diff_la, p2diff_ha,
				&la_length, &ha_length,
				high_p2diff, low_p2diff,
				low_index, high_index,
				di_para->alg_para.fmd_alg_para.hl_ratio,
				di_para->alg_para.fmd_alg_para.r_sigma3,
				di_para->alg_para.fmd_alg_para.t_mlb,
				di_para->alg_para.fmd_alg_para.t_value,
				di_para->alg_para.fmd_alg_para.r_lowcad,
				di_para->alg_para.fmd_alg_para.t_sigma3_pixel_d10
					* width * heightf / 10,
				di_para->alg_para.fmd_alg_para.r_sc,
				di_para->alg_para.fmd_alg_para.r_fb_sc,
				field_index + i - (FMD32FIELDNUM - 1));
		}
	}

	/* trace or detect */
	if (is_lock_pre) {
		/* last field lock */
		/* update phase */
		weave_phase_cur = di_alg_fmd_non22_phase(
			weave_phase_pre, film_mode_non22);

		if (p2_init_phase && init_field_cnt > 0
			&& init_field_cnt_22 > 0) {
			/* enough p2 to trace */
			is_lock_cur = di_alg_fmd_non22_p2_trace(
				weave_phase_cur, film_mode_non22,
				lowhigh[init_field_cnt - 1],
				cadence_cur[init_field_cnt_22 - 1],
				cur_totalfid,
				di_para->alg_para.fmd_alg_para.fid_trace_en,
				di_para->alg_para.fmd_alg_para.t_p1diff_trace_pixel
					* width * heightf);
		} else if (init_field_cnt_22 > 0) {
			/* not enough p2 to trace, use p1 trace */
			is_lock_cur = di_alg_fmd_non22_p1_trace(weave_phase_cur,
				film_mode_non22, cadence_cur[init_field_cnt_22 - 1]);
		}

		if (is_text_field)
			is_lock_cur = 0;

		if (is_lock_cur == 0) {
			cad_low_length = 0;
			if (p2_init_phase) {
				init_field_cnt = 0;
				la_length = 0;
				ha_length = 0;
			}
		}
	} else {
		if (p2_init_phase == 1 && is_text_field == 0)
			is_lock_cur =
			    di_alg_fmd_non22_detect(low_index, high_index,
					la_length, ha_length, lowhigh,
					&film_mode_non22, &weave_phase_cur,
					di_para->alg_para.fmd_alg_para.fmd_32_en,
					di_para->alg_para.fmd_alg_para.period32,
					di_para->alg_para.fmd_alg_para.fmd_2224_en,
					di_para->alg_para.fmd_alg_para.period2224,
					di_para->alg_para.fmd_alg_para.fmd_2332_en,
					di_para->alg_para.fmd_alg_para.period2332,
					di_para->alg_para.fmd_alg_para.fmd_32322_en,
					di_para->alg_para.fmd_alg_para.period32322,
					di_para->alg_para.fmd_alg_para.fmd_55_en,
					di_para->alg_para.fmd_alg_para.period55,
					di_para->alg_para.fmd_alg_para.fmd_64_en,
					di_para->alg_para.fmd_alg_para.period64,
					di_para->alg_para.fmd_alg_para.fmd_87_en,
					di_para->alg_para.fmd_alg_para.period87);
		else
			is_lock_cur = 0;
	}

	/* is_non22_temp_di */
	if ((is_lock_cur == 1) && (p2_init_phase == 0))
		is_non22_temp_di = 1;
	else
		is_non22_temp_di = 0;

	/* update hist */
	alg_hist->p2_init_phase = p2_init_phase;
	alg_hist->init_field_cnt_non22 = init_field_cnt;
	alg_hist->is_non22_lock = is_lock_cur;
	alg_hist->weave_phase_non22 = weave_phase_cur;
	alg_hist->film_mode_non22 = film_mode_non22;
	alg_hist->is_non22_temp_di = is_non22_temp_di;
	alg_hist->cad_low_length = cad_low_length;
	alg_hist->la_length = la_length;
	alg_hist->ha_length = ha_length;
	alg_hist->field_index = field_index;
}

static void di_alg_fmd_decision(
	struct __fmd_alg_hist *alg_hist, u32 base_field)
{
	u32 is_fm_lock;
	u32 weave_phase;
	u32 is_temp_di;
	u32 film_mode;
	u8 *fw_ph;

	if (alg_hist->is_non22_lock) {
		is_fm_lock = 1;
		film_mode = alg_hist->film_mode_non22;

		switch (alg_hist->film_mode_non22) {
		case 1:
			fw_ph = fw_ph_32;
			break;
		case 2:
			fw_ph = fw_ph_2332;
			break;
		case 3:
			fw_ph = fw_ph_2224;
			break;
		case 4:
			fw_ph = fw_ph_32322;
			break;
		case 5:
			fw_ph = fw_ph_55;
			break;
		case 6:
			fw_ph = fw_ph_64;
			break;
		case 7:
			fw_ph = fw_ph_87;
			break;
		default:
			fw_ph = fw_ph_22;
			break;
		}
		if (alg_hist->is_non22_temp_di)
			weave_phase = 1;
		else
			weave_phase = fw_ph[alg_hist->weave_phase_non22];
	} else if (alg_hist->is_22_lock) {
		is_fm_lock = 1;
		film_mode = FM_22;

		if (alg_hist->is_22_temp_di)
			weave_phase = 1;
		else
			weave_phase = fw_ph_22[alg_hist->weave_phase_22];
	} else {
		is_fm_lock = 0;
		weave_phase = 1;
		film_mode = FM_NULL;
	}

	if (alg_hist->is_22_temp_di || alg_hist->is_non22_temp_di)
		is_temp_di = 1;
	else
		is_temp_di = 0;

	/* if conflict */
	if (alg_hist->is_non22_lock && alg_hist->is_22_lock) {
		/* except 2-2 and 2-2-2-4/ 6-4 conflict is allowed */
		if (alg_hist->film_mode_non22 != FM_2224 &&
		    alg_hist->film_mode_non22 != FM_64) {
			is_fm_lock = 0;
			alg_hist->is_non22_lock = 0;
			alg_hist->is_22_lock = 0;
		}
	}

	DI_DEBUG("base_field=%d, is_fm_lock=%d\n", base_field, is_fm_lock);
	if (base_field) {
		alg_hist->is_fm_lock_f4 = is_fm_lock;
		alg_hist->weave_phase_f4 = weave_phase;
		alg_hist->is_temp_di_f4 = is_temp_di;
		alg_hist->film_mode_f4 = film_mode;
	} else {
		alg_hist->is_fm_lock_f3 = is_fm_lock;
		alg_hist->weave_phase_f3 = weave_phase;
		alg_hist->is_temp_di_f3 = is_temp_di;
		alg_hist->film_mode_f3 = film_mode;
	}
}

static void di_alg_film_mode_detection(
	struct __fmd_hist *fmd_hist,
	struct __di_para_t *di_para,
	struct __alg_hist *alg_hist,
	u32 base_field)
{
	u32 is_fo_change;
	u32 is_text_field;

	is_fo_change = alg_hist->fod_alg_hist.is_fieldorderchange;
	is_text_field = alg_hist->vof_alg_hist.text_field_exist_f3 |
	    alg_hist->vof_alg_hist.text_field_exist_f4;

	/* p1diff update/ scene change detect/ 2-2 film mode detect and trace */
	di_alg_fmd_22(fmd_hist, di_para, &alg_hist->fmd_alg_hist,
		is_fo_change, is_text_field, base_field);

	/* p2diff update/ non 2-2 film mode detect and trace */
	di_alg_fmd_non22(fmd_hist, di_para, &alg_hist->fmd_alg_hist,
		is_fo_change, is_text_field, base_field);

	/* film mode decision */
	di_alg_fmd_decision(&alg_hist->fmd_alg_hist, base_field);
}

static void di_alg_itd_22(struct __itd_hist *itd_hist,
	struct __di_para_t *di_para, struct __itd_alg_hist *alg_hist,
	u32 is_fo_change, u32 is_text_field, u32 base_field)
{
	u32 p1_init_phase = alg_hist->p1_init_phase;
	u32 init_field_cnt_22 = alg_hist->init_field_cnt_22;
	u32 is_progressive_lock_pre = alg_hist->is_progressive_lock;
	s32 sc_trace_cnt = alg_hist->sc_trace_cnt;
	u32 sc_length = alg_hist->sc_length;
	u32 pre_totalfid = alg_hist->pre_totalfid;
	u32 cur_totalfid =
	    (base_field) ? itd_hist->FMD_FID23 : itd_hist->FMD_FID12;
	u32 cad_low_length = alg_hist->cad_low_length;
	u32 weave_phase_22_pre = alg_hist->weave_phase_22;
	u32 weave_phase_22_cur = 0;
	u32 is_progressive_lock_cur;
	u32 is_scenechange;
	u32 is_scenetrace;
	u32 is_22_temp_di;
	u32 cadence_cur;
	u32 weave_phase = 0;

	s32 i, width, heightf;
	u32 t_p1diffthrl, t_p1diffthrh, avgns;
	u32 t_p1diff, t_p1diff_u;

	width = di_para->width;
	heightf = di_para->height / 2;

	/* if no pre field fid, pass the detection */
	if (alg_hist->pre_totalfid_valid == 0)
		goto ALG_FMD_22_END;

	/* Calculate cadence_cur */
	t_p1diffthrl = di_para->alg_para.itd_alg_para.t_p1diffthrl_pixel
		* width * heightf;
	t_p1diffthrh = di_para->alg_para.itd_alg_para.t_p1diffthrh_pixel
		* width * heightf;
	avgns = di_para->alg_para.itd_alg_para.avgns_pixel * width * heightf;

	di_alg_fmd_p1(cur_totalfid, pre_totalfid, t_p1diffthrl, t_p1diffthrh,
		avgns, di_para->alg_para.itd_alg_para.r_film22rel,
		di_para->alg_para.itd_alg_para.r_film22rel2,
		alg_hist->cad_low_array, &cad_low_length, &cadence_cur);

	/* Update cadence_cur */
	if (init_field_cnt_22 < FMD22FIELDNUM) {
		alg_hist->cadence_cur[init_field_cnt_22] = cadence_cur;
	} else {
		for (i = 0; i < FMD22FIELDNUM - 1; i++)
			alg_hist->cadence_cur[i] = alg_hist->cadence_cur[i + 1];
		alg_hist->cadence_cur[FMD22FIELDNUM - 1] = cadence_cur;
	}

	p1_init_phase = (init_field_cnt_22 < 1) ? 0 : 1;
	if (init_field_cnt_22 < FMD22FIELDNUM)
		init_field_cnt_22++;

	t_p1diff = di_para->alg_para.itd_alg_para.t_p1diff_pixel
	    * width * heightf;
	t_p1diff_u = di_para->alg_para.itd_alg_para.t_p1diff_u_pixel
		* width * heightf;
	is_scenechange = di_alg_fmd_sc_detect(alg_hist->sc_array,
		&sc_length, cur_totalfid,
		di_para->alg_para.itd_alg_para.r_sc,
		t_p1diff, t_p1diff_u, cadence_cur);

	if (is_scenechange)
		init_field_cnt_22 = 0;

	/* trace or detect */
	is_scenetrace = (is_progressive_lock_pre && sc_trace_cnt > 0) ? 1 : 0;
	if (is_progressive_lock_pre) {
		/* if last field 2-2 lock */
		is_progressive_lock_cur = di_alg_fmd_22_trace(weave_phase_22_pre,
			&weave_phase_22_cur, cadence_cur,
			is_scenechange, is_scenetrace);

		/* when text exist, all film mode unlock */
		if (is_text_field)
			is_progressive_lock_cur = 0;

		if (is_progressive_lock_cur) {
			if (is_scenechange)
				sc_trace_cnt =
				    di_para->alg_para.itd_alg_para.sc_trace_fnum;

			if (is_scenetrace)
				sc_trace_cnt -= 1;
		} else {
			init_field_cnt_22 = 0;
			sc_trace_cnt = 0;
			cad_low_length = 0;
		}
	} else {
		if (p1_init_phase == 1 && is_text_field == 0)
			is_progressive_lock_cur = di_alg_fmd_22_detect(
				alg_hist->cadence_cur, init_field_cnt_22,
				&weave_phase_22_cur,
				di_para->alg_para.itd_alg_para.period22_x0,
				di_para->alg_para.itd_alg_para.period22_10, 1);
		else
			is_progressive_lock_cur = 0;
	}

	/* is_22_temp_di */
	if (is_progressive_lock_cur
		&& (is_scenechange || is_scenetrace))
		is_22_temp_di = 1;
	else
		is_22_temp_di = 0;

	if (is_progressive_lock_cur) {
		if (is_22_temp_di)
			weave_phase = 1;
		else
			weave_phase = fw_ph_22[is_progressive_lock_cur];
	}

	/* update hist */
	alg_hist->p1_init_phase = p1_init_phase;
	alg_hist->init_field_cnt_22 = init_field_cnt_22;
	alg_hist->is_progressive_lock = is_progressive_lock_cur;
	alg_hist->sc_trace_cnt = sc_trace_cnt;
	alg_hist->sc_length = sc_length;
	alg_hist->cad_low_length = cad_low_length;
	alg_hist->weave_phase_22 = weave_phase_22_cur;

	if (base_field)
		alg_hist->is_scenechange_f4 = is_scenechange;
	else
		alg_hist->is_scenechange_f3 = is_scenechange;

	if (base_field)
		alg_hist->is_scenechange_f4 = is_scenechange;
	else
		alg_hist->is_scenechange_f3 = is_scenechange;

	if (base_field) {
		alg_hist->weave_phase_f4 = weave_phase;
		alg_hist->is_temp_di_f4 = is_22_temp_di;
	} else {
		alg_hist->weave_phase_f3 = weave_phase;
		alg_hist->is_temp_di_f3 = is_22_temp_di;
	}

ALG_FMD_22_END:
	alg_hist->pre_totalfid = cur_totalfid;
	alg_hist->pre_totalfid_valid = 1;
}

static void di_alg_iterlace_detection(
	struct __itd_hist *itd_hist,
	struct __di_para_t *di_para,
	struct __alg_hist *alg_hist,
	u32 base_field)
{
	u32 is_fo_change;
	u32 is_text_field;

	is_fo_change = alg_hist->fod_alg_hist.is_fieldorderchange;
	is_text_field = alg_hist->vof_alg_hist.text_field_exist_f3
	    || alg_hist->vof_alg_hist.text_field_exist_f4;

	/* p1diff update/ scene change detect/ 2-2 film mode detect and trace */
	di_alg_itd_22(itd_hist, di_para, &alg_hist->itd_alg_hist,
		is_fo_change, is_text_field, base_field);
}

/*******************************************************************/
/*                 Video-On-Film Detection Algorithm               */
/*******************************************************************/
static void di_alg_video_field(
	struct __fmd_hist *fmd_hist,
	struct __di_para_t *di_para,
	struct __vof_alg_hist *alg_hist,
	u32 base_field)
{
	struct __vof_alg_para *alg_para = &di_para->alg_para.vof_alg_para;
	s32 field_num;
	s32 i;
	u32 sum_field_num, sum_field_num5;
	u32 *pre_field_status, *cur_field_status;
	s32 larger_than_th0_field_cnt5, larger_than_th0_field_cnt10;
	s32 larger_than_zero_field_cnt10;

	field_num = alg_para->fade_out_video_field_num;

	pre_field_status = (base_field == 0) ? &alg_hist->video_field_exist_f4 :
	    &alg_hist->video_field_exist_f3;

	cur_field_status = (base_field == 0) ? &alg_hist->video_field_exist_f3 :
	    &alg_hist->video_field_exist_f4;
	/* update field_max_video_num_array */
	for (i = 0; i < field_num - 1; i++)
		alg_hist->field_max_video_num_array[i] =
		    alg_hist->field_max_video_num_array[i + 1];

	if (field_num > 0)
		alg_hist->field_max_video_num_array[field_num - 1] = (base_field == 0) ?
			fmd_hist->FIELD_MAX_VIDEO_NUM_F3 : fmd_hist->FIELD_MAX_VIDEO_NUM_F4;

	larger_than_th0_field_cnt10 = 0;
	larger_than_zero_field_cnt10 = 0;
	sum_field_num = 0;
	for (i = 0; i < field_num; i++) {
		sum_field_num += alg_hist->field_max_video_num_array[i];
		larger_than_th0_field_cnt10 +=
		    alg_hist->field_max_video_num_array[i] >
		    alg_para->video_field_th0 ? 1 : 0;
		larger_than_zero_field_cnt10 +=
		    alg_hist->field_max_video_num_array[i] > 0 ? 1 : 0;
	}

	larger_than_th0_field_cnt5 = 0;
	sum_field_num5 = 0;
	for (i = field_num - 1; i >= field_num - 5; i--) {
		sum_field_num5 += alg_hist->field_max_video_num_array[i];
		larger_than_th0_field_cnt5 +=
		    alg_hist->field_max_video_num_array[i] >
		    alg_para->video_field_th0 ? 1 : 0;
	}

	if (*pre_field_status) {	/* video exist last field */
		/* Last field_num field don't exist any video row */
		if (sum_field_num == 0)
			*cur_field_status = 0;
		else		/* remain video exist this field */
			*cur_field_status = 1;
	} else {
		if (sum_field_num5 > alg_para->video_field_th1 &&
		    larger_than_th0_field_cnt5 == 5) {
			*cur_field_status = 1;
		} else {
			if (sum_field_num > alg_para->video_field_th2 &&
			    larger_than_th0_field_cnt10 > 0 &&
			    larger_than_zero_field_cnt10 == 10)
				*cur_field_status = 1;
			else
				*cur_field_status = 0;
		}
	}
}

static void di_alg_text_field(struct __fmd_hist *fmd_hist,
	struct __di_para_t *di_para,
	struct __vof_alg_hist *alg_hist,
	u32 base_field)
{
	struct __vof_alg_para *alg_para = &di_para->alg_para.vof_alg_para;
	s32 fade_out_field_num, fade_in_field_num;
	s32 i;
	u32 sum_field_num, max_blk_pos;
	u32 *pre_field_status, *cur_field_status;
	s32 larger_than_th1_field_cnt, larger_than_th2_field_cnt;
	s32 blksize = di_para->blksize;

	fade_out_field_num = alg_para->fade_out_text_field_num;
	fade_in_field_num = alg_para->fade_in_text_field_num;

	pre_field_status = (base_field == 0) ? &alg_hist->text_field_exist_f4 :
	    &alg_hist->text_field_exist_f3;

	cur_field_status = (base_field == 0) ? &alg_hist->text_field_exist_f3 :
	    &alg_hist->text_field_exist_f4;

	max_blk_pos = (base_field == 0) ? fmd_hist->FIELD_MAX_TEXT_POS_F3 :
	    fmd_hist->FIELD_MAX_TEXT_POS_F4;

	/* update field_max_text_num_array */
	for (i = 0; i < fade_out_field_num - 1; i++)
		alg_hist->field_max_text_num_array[i] =
		    alg_hist->field_max_text_num_array[i + 1];

	if (fade_out_field_num > 0)
		alg_hist->field_max_text_num_array[fade_out_field_num - 1] =
			(base_field == 0) ? fmd_hist->FIELD_MAX_TEXT_NUM_F3 :
			fmd_hist->FIELD_MAX_TEXT_NUM_F4;

	/* update field_text_num_array */
	for (i = 0; i < fade_out_field_num - 1; i++)
		alg_hist->field_text_num_array[i] =
		    alg_hist->field_text_num_array[i + 1];

	if (fade_out_field_num > 0)
		alg_hist->field_text_num_array[fade_out_field_num - 1] =
			(base_field == 0) ? fmd_hist->FIELD_TEXT_ROW_NUM_F3 :
			fmd_hist->FIELD_TEXT_ROW_NUM_F4;

	sum_field_num = 0;
	larger_than_th1_field_cnt = 0;
	larger_than_th2_field_cnt = 0;
	for (i = 0; i < fade_out_field_num; i++) {
		sum_field_num += alg_hist->field_max_text_num_array[i];
		larger_than_th1_field_cnt +=
		    alg_hist->field_max_text_num_array[i] >
		    alg_para->text_field_th1 ? 1 : 0;
		larger_than_th2_field_cnt +=
		    alg_hist->field_text_num_array[i] >
		    alg_para->text_field_th2 ? 1 : 0;

	}

	if (*pre_field_status == 0) {
		if ((larger_than_th1_field_cnt ==
		     alg_para->fade_in_text_field_num) &&
		    (sum_field_num > alg_para->text_field_th0) &&
		    (larger_than_th2_field_cnt ==
		     alg_para->fade_in_text_field_num) &&
		    (max_blk_pos * 2 * blksize > alg_para->text_field_pos_u) &&
		    (max_blk_pos * 2 * blksize < alg_para->text_field_pos_l))
			*cur_field_status = 1;
		else
			*cur_field_status = 0;
	} else {
		if (larger_than_th1_field_cnt == 0)
			*cur_field_status = 0;
		else
			*cur_field_status = 1;
	}

}

static void di_alg_video_on_film_detection(
	struct __fmd_hist *fmd_hist,
	struct __di_para_t *di_para,
	struct __vof_alg_hist *alg_hist)
{
	/* Field 3 */
	di_alg_video_field(fmd_hist, di_para, alg_hist, 0);
	di_alg_text_field(fmd_hist, di_para, alg_hist, 0);

	/* Field 4 */
	di_alg_video_field(fmd_hist, di_para, alg_hist, 1);
	di_alg_text_field(fmd_hist, di_para, alg_hist, 1);
}

static void di_alg_tnr_strengths_adjustment(
	struct __tnr_hist *tnr_hist,
	struct __di_para_t *di_para,
	struct __alg_hist *alg_hist)
{
	u32 gain_next;

	if ((alg_hist->fmd_alg_hist.is_scenechange_f3 != 1) &&
	    (alg_hist->fmd_alg_hist.is_scenechange_f4 != 1)) {

		u64 noise_weight_num, noise_weight_den;
		u64 still_weight_num, still_weight_den;
		u64 cur_weight, den;

		/*
		* u64 noise_level, noise_predict;
		* noise_level = tnr_hist->tnr_sum_gain_y
		*	/ tnr_hist->tnr_sum_gain_y_cnt;
		* noise_predict = alg_hist->tnr_alg_hist.gain *
		*	di_para->alg_para.tnr_alg_para.k_max_noise_num * 64
		*	/ 63 / di_para->alg_para.tnr_alg_para.k_max_noise_den;
		* noise_weight = noise_level / noise_predict;
		*/
		noise_weight_num = tnr_hist->tnr_sum_gain_y
			* di_para->alg_para.tnr_alg_para.k_max_noise_den * 63;
		noise_weight_den = tnr_hist->tnr_sum_gain_y_cnt
			* alg_hist->tnr_alg_hist.gain
			* di_para->alg_para.tnr_alg_para.k_max_noise_num * 64;

		/*
		* u64 still_level;
		* still_level = tnr_hist->tnr_sum_still_out
		*	/ (di_para->width * di_para->height);
		* still_weight = still_level * di_para->alg_para.tnr_alg_para.k_max_still_den
		*	/ di_para->alg_para.tnr_alg_para.k_max_still_num;
		*/
		still_weight_num = tnr_hist->tnr_sum_still_out
			* di_para->alg_para.tnr_alg_para.k_max_still_den;
		still_weight_den = di_para->width * di_para->height
			* di_para->alg_para.tnr_alg_para.k_max_still_num;

		cur_weight = noise_weight_num * still_weight_num;
		den = noise_weight_den * still_weight_den;
		if (den != 0) {
			do_div(cur_weight, den);
		} else {
			cur_weight = 0;
			DI_ERR("ERR: %s zero den\n", __func__);
		}

		gain_next = ((u32)cur_weight + 31 * alg_hist->tnr_alg_hist.gain) / 32;
		gain_next = max(di_para->alg_para.tnr_alg_para.tnr_adaptive_gain_th_l, gain_next);
		gain_next = min(di_para->alg_para.tnr_alg_para.tnr_adaptive_gain_th, gain_next);
	} else {
		gain_next = di_para->alg_para.tnr_alg_para.tnr_adaptive_gain_th;
	}

	alg_hist->tnr_alg_hist.gain = gain_next;
}

/*******************************************************************/
/*                      DI300 Algorithm Top                        */
/*******************************************************************/
void di_alg(struct di_dev_proc_result *proc_rst)
{
	struct __hw_hist *hw_hist = &proc_rst->hw_hist;
	struct __alg_hist *alg_hist = &proc_rst->alg_hist;
	struct __di_para_t *di_para = &proc_rst->di_para;

	if (di_para->alg_para.alg_en) {
		/* TODO : add itd and tnr alg para initial */
		if (di_para->alg_para.itd_alg_para.itd_alg_en) {
			di_alg_iterlace_detection(&hw_hist->itd_hist,
				di_para, alg_hist, 0);
			di_alg_iterlace_detection(&hw_hist->itd_hist,
				di_para, alg_hist, 1);
		}

		if (di_para->alg_para.fod_alg_para.fod_alg_en) {
			di_alg_field_order_detection(&hw_hist->fmd_hist,
				di_para, &alg_hist->fod_alg_hist);
		}

		if (di_para->alg_para.vof_alg_para.vof_alg_en) {
			di_alg_video_on_film_detection(&hw_hist->fmd_hist,
				di_para, &alg_hist->vof_alg_hist);
		}

		if (di_para->alg_para.fmd_alg_para.fmd_alg_en) {
			di_alg_film_mode_detection(&hw_hist->fmd_hist,
				di_para, alg_hist, 0);
			di_alg_film_mode_detection(&hw_hist->fmd_hist,
				di_para, alg_hist, 1);
		}

		if (di_para->alg_para.tnr_alg_para.tnr_alg_en) {
			di_alg_tnr_strengths_adjustment(
				&hw_hist->tnr_hist, di_para, alg_hist);
		}
	}
}

/*******************************************************************/
/*            Algorithm and Hardware Data Exchange                 */
/*******************************************************************/

/* Set algorithm parameters from hardware setting */
void di_alg_fixed_para(struct di_client *c,
	struct di_dev_proc_result *proc_rst)
{
	struct __di_para_t *di_para = &proc_rst->di_para;
	struct __alg_para_t *alg_para = &di_para->alg_para;
	struct __alg_hist *alg_hist = &proc_rst->alg_hist;

	di_para->bff = c->fb_arg.top_field_first ? 0 : 1;
	di_para->width = c->video_size.width;
	di_para->height = c->video_size.height;
	di_para->blksize = c->vof_blk_size_sel ? 16 : 8;

	alg_para->fod_alg_para.fod_alg_en = (c->mode == DI_MODE_60HZ) ||
	    (c->mode == DI_MODE_30HZ) || (c->mode == DI_MODE_WEAVE);

	/* Enable FMD and VOF when scan type of video information is interlaced
	 * No matter the progressive video is detected to be a interlaced video
	 */
	alg_para->fmd_alg_para.fmd_alg_en = (c->mode == DI_MODE_60HZ) &&
	    (c->fb_arg.is_interlace == 1);

	alg_para->vof_alg_para.vof_alg_en = (c->mode == DI_MODE_60HZ) &&
	    (c->fb_arg.is_interlace == 1);

	/* Enable ITD when scan type of video information is progressive */
	alg_para->itd_alg_para.itd_alg_en =
		((c->mode == DI_MODE_60HZ) ||
		(c->mode == DI_MODE_30HZ) ||
		(c->mode == DI_MODE_WEAVE));

	/* Enable TNR adaptive gain control when TNR module enable */
	alg_para->tnr_alg_para.tnr_alg_en = c->tnr_en;

	alg_para->alg_en = 1;

	alg_para->fod_alg_para.R_kick = 5;
	alg_para->fod_alg_para.R_rev_10 = 11;
	alg_para->fod_alg_para.T_rev = 0;
	alg_para->fod_alg_para.T_rev_time = 15;

	alg_para->vof_alg_para.fade_out_video_field_num = 10;
	alg_para->vof_alg_para.video_field_th0 = 3 * di_para->width *
	    di_para->height / 720 / 480;
	alg_para->vof_alg_para.video_field_th1 = 25 * di_para->width *
	    di_para->height / 720 / 480;
	alg_para->vof_alg_para.video_field_th2 = 15 * di_para->width *
	    di_para->height / 720 / 480;
	alg_para->vof_alg_para.fade_in_text_field_num = 20;
	alg_para->vof_alg_para.fade_out_text_field_num = 59;
	alg_para->vof_alg_para.text_field_th0 = 20 * 3 * di_para->width *
	    di_para->height / 720 / 480;
	alg_para->vof_alg_para.text_field_th1 = 3 * di_para->width *
	    di_para->height / 720 / 480;
	alg_para->vof_alg_para.text_field_th2 = 0;	/* ignored first */
	alg_para->vof_alg_para.text_field_pos_u = di_para->height / 4;
	alg_para->vof_alg_para.text_field_pos_l = 3 * di_para->height / 4;

	alg_para->fmd_alg_para.fmd_55_en = 1;
	alg_para->fmd_alg_para.fmd_64_en = 1;
	alg_para->fmd_alg_para.fmd_87_en = 1;
	alg_para->fmd_alg_para.fmd_2332_en = 1;
	alg_para->fmd_alg_para.fmd_2224_en = 1;
	alg_para->fmd_alg_para.fmd_32322_en = 1;
	alg_para->fmd_alg_para.fmd_32_en = 1;
	alg_para->fmd_alg_para.fmd_22_en = 1;

	alg_para->fmd_alg_para.r_sc = 2;
	alg_para->fmd_alg_para.hl_ratio = 6;
	alg_para->fmd_alg_para.r_sigma3 = 1;
	alg_para->fmd_alg_para.t_mlb = 2;
	alg_para->fmd_alg_para.t_value = 300000000;
	alg_para->fmd_alg_para.r_lowcad = 2;
	alg_para->fmd_alg_para.r_fb_sc = 2;
	alg_para->fmd_alg_para.t_sigma3_pixel_d10 = 1;
	alg_para->fmd_alg_para.fid_trace_en = 1;
	alg_para->fmd_alg_para.t_p1diff_trace_pixel = 2;

	alg_para->fmd_alg_para.t_p1diffthrl_pixel = 20;
	alg_para->fmd_alg_para.t_p1diffthrh_pixel = 40;
	alg_para->fmd_alg_para.avgns_pixel = 0;
	alg_para->fmd_alg_para.r_film22rel = 3;
	alg_para->fmd_alg_para.r_film22rel2 = 3;
	alg_para->fmd_alg_para.sc_trace_fnum = 11;

	alg_para->fmd_alg_para.t_p1diff_pixel = 20;
	alg_para->fmd_alg_para.t_p1diff_u_pixel = 40;

	alg_para->fmd_alg_para.period22_x0 = 25;
	alg_para->fmd_alg_para.period22_10 = 20;

	alg_para->fmd_alg_para.period32 = 3;
	alg_para->fmd_alg_para.period2224 = 3;
	alg_para->fmd_alg_para.period2332 = 3;
	alg_para->fmd_alg_para.period32322 = 3;
	alg_para->fmd_alg_para.period55 = 3;
	alg_para->fmd_alg_para.period64 = 3;
	alg_para->fmd_alg_para.period87 = 3;

	alg_para->itd_alg_para.itd_alg_en = 1;
	alg_para->itd_alg_para.r_sc = 2;
	alg_para->itd_alg_para.t_p1diffthrl_pixel = 20;
	alg_para->itd_alg_para.t_p1diffthrh_pixel = 40;
	alg_para->itd_alg_para.avgns_pixel = 0;
	alg_para->itd_alg_para.r_film22rel = 3;
	alg_para->itd_alg_para.r_film22rel2 = 3;
	alg_para->itd_alg_para.sc_trace_fnum = 11;	/* sc_trace_fieldnum */
	alg_para->itd_alg_para.t_p1diff_pixel = 20;
	alg_para->itd_alg_para.t_p1diff_u_pixel = 40;
	alg_para->itd_alg_para.period22_x0 = 25;	/* required period to detect 2-2 */
	alg_para->itd_alg_para.period22_10 = 20;	/* required period to detect 2-2 */

	/* TODO : add itd and tnr alg para initial */
	alg_para->tnr_alg_para.tnr_mode = c->tnr_mode.mode;
	if (c->tnr_mode.level == DI_TNR_LEVEL_LOW) {
		alg_para->tnr_alg_para.tnr_adaptive_gain_th = 38;
		alg_para->tnr_alg_para.tnr_adaptive_gain_th_l = 16;
	} else if (c->tnr_mode.level == DI_TNR_LEVEL_MIDDLE) {
		alg_para->tnr_alg_para.tnr_adaptive_gain_th = 48;
		alg_para->tnr_alg_para.tnr_adaptive_gain_th_l = 28;
	} else {
		alg_para->tnr_alg_para.tnr_adaptive_gain_th = 62;
		alg_para->tnr_alg_para.tnr_adaptive_gain_th_l = 52;
	}

	alg_para->tnr_alg_para.k_max_still_num = 5;
	alg_para->tnr_alg_para.k_max_still_den = 2;
	alg_para->tnr_alg_para.k_max_noise_num = 2;
	alg_para->tnr_alg_para.k_max_noise_den = 1;

	alg_hist->tnr_alg_hist.gain =
	    alg_para->tnr_alg_para.tnr_adaptive_gain_th;
}

void di_alg_hist_to_hardware(struct di_client *c,
	struct di_dev_proc_result *proc_rst)
{
	struct __alg_hist *alg_hist = &proc_rst->alg_hist;
	struct __di_para_t *di_para = &proc_rst->di_para;
	struct __alg_para_t *alg_para = &di_para->alg_para;

	if (alg_para->alg_en == 0)
		return;

	if (alg_para->fod_alg_para.fod_alg_en) {
		if (alg_hist->fod_alg_hist.is_fieldorderchange) {
			di_para->bff = alg_hist->fod_alg_hist.bff_fix;
			DI_INFO("is_fieldorderchange, bff:%d\n", di_para->bff);
		}
	}
}
