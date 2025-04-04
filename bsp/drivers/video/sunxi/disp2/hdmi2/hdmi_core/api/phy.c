/*
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include "phy.h"
#include "util.h"
#include "core/irq.h"

#define I2C_TIMEOUT				100

#ifdef SUPPORT_PHY_JTAG
static void phy_jtag_reset(hdmi_tx_dev_t *dev);
static void phy_jtag_slave_address(hdmi_tx_dev_t *dev, u8 jtag_addr);
#endif
static void phy_interrupt_mask(hdmi_tx_dev_t *dev, u8 mask);

static int phy_phase_lock_loop_state(hdmi_tx_dev_t *dev);
static int phy_reconfigure_interface(hdmi_tx_dev_t *dev);
static int phy_slave_address(hdmi_tx_dev_t *dev, u8 value);


#if 0
void phy_test_clear(hdmi_tx_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, PHY_TST0, PHY_TST0_SPARE_4_MASK, (bit ? 1 : 0));
}

static void phy_test_enable(hdmi_tx_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, PHY_TST0, PHY_TST0_SPARE_3_MASK, (bit ? 1 : 0));
}

static void phy_test_clock(hdmi_tx_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, PHY_TST0, PHY_TST0_SPARE_0_MASK, (bit ? 1 : 0));
}

static void phy_test_data_in(hdmi_tx_dev_t *dev, u8 data)
{
	LOG_TRACE1(data);
	dev_write(dev, (PHY_TST1), data);
}

u8 phy_test_data_out(hdmi_tx_dev_t *dev, u32 baseAddr)
{
	LOG_TRACE();
	return dev_read(dev, PHY_TST2);
}

u8 phy_interrupt_state(hdmi_tx_dev_t *dev)
{
	LOG_TRACE();
	return dev_read(dev, PHY_INT0);
}

u8 phy_interrupt_mask_status(hdmi_tx_dev_t *dev, u8 mask)
{
	LOG_TRACE1(mask);
	return dev_read(dev, PHY_MASK0) & mask;
}

void phy_interrupt_polarity(hdmi_tx_dev_t *dev, u8 bitShift, u8 value)
{
	LOG_TRACE2(bitShift, value);
	dev_write_mask(dev, PHY_POL0, (1 << bitShift), value);
}

u8 phy_interrupt_polarity_status(hdmi_tx_dev_t *dev, u8 mask)
{
	LOG_TRACE1(mask);
	return dev_read(dev, PHY_POL0) & mask;
}
#endif

#if 0
static int phy301_configure_supported(hdmi_tx_dev_t *dev, u32 pClk,
				color_depth_t color, pixel_repetition_t pixel)
{
	int i   = 0;

	/* Color resolution 0 is 8 bit color depth */
	if (color == 0)
		color = COLOR_DEPTH_8;

	pClk = phy301_get_freq(pClk);

	for (i = 0; phy301[i].clock != 0; i++) {
		if ((pClk == phy301[i].clock) &&
				(color == phy301[i].color) &&
				(pixel == phy301[i].pixel)) {
			return TRUE;
		}
	}

	return FALSE;
}

static int phy303_configure_supported(hdmi_tx_dev_t *dev, u32 pClk,
			color_depth_t color, pixel_repetition_t pixel)
{
	int i   = 0;

	/* Color resolution 0 is 8 bit color depth */
	if (color == 0)
		color = COLOR_DEPTH_8;

	pClk = phy303_get_freq(pClk);

	for (i = 0; phy303[i].clock != 0; i++) {
		if ((pClk == phy303[i].clock) &&
				(color == phy303[i].color) &&
				(pixel == phy303[i].pixel)) {
			return TRUE;
		}
	}

	return FALSE;
}
#endif

#if 0
int phy_configuration_supported(hdmi_tx_dev_t *dev, u16 phy_model)
{
	if (phy_model == PHY_MODEL_301) {
		return phy301_configure_supported(dev,
					dev->snps_hdmi_ctrl.pixel_clock,
					dev->snps_hdmi_ctrl.color_resolution,
					dev->snps_hdmi_ctrl.pixel_repetition);
	}
	if (phy_model == PHY_MODEL_303) {
		return phy303_configure_supported(dev,
					dev->snps_hdmi_ctrl.pixel_clock,
					dev->snps_hdmi_ctrl.color_resolution,
					dev->snps_hdmi_ctrl.pixel_repetition);
	}
	pr_err("****** PHY not supported %d *******\n", phy_model);
	return FALSE;

}
#endif

#if 0
int phy_hot_plug_detected(hdmi_tx_dev_t *dev)
{
	/* MASK         STATUS          POLARITY        INTERRUPT        HPD
	 *   0             0                 0               1             0
	 *   0             1                 0               0             1
	 *   0             0                 1               0             0
	 *   0             1                 1               1             1
	 *   1             x                 x               0             x
	 */

	int hpd_polarity = dev_read_mask(dev, PHY_POL0, PHY_POL0_HPD_MASK);
	int hpd = dev_read_mask(dev, PHY_STAT0, PHY_STAT0_HPD_MASK);

	/* Mask interrupt */
	phy_interrupt_mask(dev, PHY_MASK0_HPD_MASK);

	if (hpd_polarity == hpd) {
		dev_write_mask(dev, PHY_POL0, PHY_POL0_HPD_MASK, !hpd_polarity);

		/* Un-mask interrupts */
		phy_interrupt_unmask(dev, PHY_MASK0_HPD_MASK);

		return hpd_polarity;
	}

	/* Un-mask interrupts */
	phy_interrupt_unmask(dev, PHY_MASK0_HPD_MASK);

	return !hpd_polarity;
}


int phy_interrupt_enable(hdmi_tx_dev_t *dev, u8 value)
{
	LOG_TRACE1(value);
	dev_write(dev, PHY_MASK0, value);
	return TRUE;
}
#endif

#if 0
int phy_test_control(hdmi_tx_dev_t *dev, u8 value)
{
	LOG_TRACE1(value);
	phy_test_data_in(dev, value);
	phy_test_enable(dev, 1);
	phy_test_clock(dev, 1);
	phy_test_clock(dev, 0);
	phy_test_enable(dev, 0);
	return TRUE;
}

int phy_test_data(hdmi_tx_dev_t *dev, u8 value)
{
	LOG_TRACE1(value);
	phy_test_data_in(dev, value);
	phy_test_enable(dev, 0);
	phy_test_clock(dev, 1);
	phy_test_clock(dev, 0);
	return TRUE;
}
#endif

void phy_set_reg_base(uintptr_t base)
{
	return;
}

uintptr_t phy_get_reg_base(void)
{
	return 0;
}

static void phy_power_down(hdmi_tx_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, PHY_CONF0, PHY_CONF0_SPARES_2_MASK, (bit ? 1 : 0));
}

static void phy_enable_tmds(hdmi_tx_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, PHY_CONF0, PHY_CONF0_SPARES_1_MASK, (bit ? 1 : 0));
}

static void phy_gen2_pddq(hdmi_tx_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, PHY_CONF0, PHY_CONF0_PDDQ_MASK, (bit ? 1 : 0));
}

static void phy_gen2_tx_power_on(hdmi_tx_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, PHY_CONF0, PHY_CONF0_TXPWRON_MASK, (bit ? 1 : 0));
}

static void phy_gen2_en_hpd_rx_sense(hdmi_tx_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, PHY_CONF0, PHY_CONF0_ENHPDRXSENSE_MASK,
					(bit ? 1 : 0));
}

static void phy_data_enable_polarity(hdmi_tx_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, PHY_CONF0, PHY_CONF0_SELDATAENPOL_MASK,
					(bit ? 1 : 0));
}

static void phy_interface_control(hdmi_tx_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, PHY_CONF0, PHY_CONF0_SELDIPIF_MASK, (bit ? 1 : 0));
}



void phy_i2c_fast_mode(hdmi_tx_dev_t *dev, u8 bit)
{
	dev_write_mask(dev, PHY_I2CM_DIV, PHY_I2CM_DIV_FAST_STD_MODE_MASK, bit);
}

void phy_i2c_master_reset(hdmi_tx_dev_t *dev)
{
	LOG_TRACE();
	dev_write_mask(dev, PHY_I2CM_SOFTRSTZ,
			PHY_I2CM_SOFTRSTZ_I2C_SOFTRSTZ_MASK, 1);
}

static void phy_i2c_mask_interrupts(hdmi_tx_dev_t *dev, int mask)
{
	LOG_TRACE1(mask);
	dev_write_mask(dev, PHY_I2CM_INT,
			PHY_I2CM_INT_DONE_MASK_MASK, mask ? 1 : 0);
	dev_write_mask(dev, PHY_I2CM_CTLINT,
			PHY_I2CM_CTLINT_ARBITRATION_MASK_MASK, mask ? 1 : 0);
	dev_write_mask(dev, PHY_I2CM_CTLINT,
			PHY_I2CM_CTLINT_NACK_MASK_MASK, mask ? 1 : 0);
}

static void phy_i2c_slave_address(hdmi_tx_dev_t *dev, u8 value)
{
	LOG_TRACE1(value);
	dev_write_mask(dev, PHY_I2CM_SLAVE,
			PHY_I2CM_SLAVE_SLAVEADDR_MASK, value);
}

int phy_i2c_write(hdmi_tx_dev_t *dev, u8 addr, u16 data)
{
	int timeout = PHY_TIMEOUT;
	u32 status  = 0;

	LOG_TRACE2(data, addr);

	/* Set address */
	dev_write(dev, PHY_I2CM_ADDRESS, addr);

	/* Set value */
	dev_write(dev, PHY_I2CM_DATAO_1, (u8) ((data >> 8) & 0xFF));
	dev_write(dev, PHY_I2CM_DATAO_0, (u8) (data & 0xFF));

	dev_write(dev, PHY_I2CM_OPERATION, PHY_I2CM_OPERATION_WR_MASK);

	do {
		snps_sleep(10);
		status = dev_read_mask(dev, IH_I2CMPHY_STAT0,
				IH_I2CMPHY_STAT0_I2CMPHYERROR_MASK |
				IH_I2CMPHY_STAT0_I2CMPHYDONE_MASK);
	} while (status == 0 && (timeout--));

	dev_write(dev, IH_I2CMPHY_STAT0, status); /* clear read status */

	if (status & IH_I2CMPHY_STAT0_I2CMPHYERROR_MASK) {
		pr_err("Error: I2C PHY write failed\n");
		return -1;
	}

	if (status & IH_I2CMPHY_STAT0_I2CMPHYDONE_MASK)
		return 0;

	pr_warn("Error: ASSERT I2C Write timeout - check PHY - exiting\n");
	return -1;
}

int phy_i2c_read(hdmi_tx_dev_t *dev, u8 addr, u16 *value)
{
	int timeout = PHY_TIMEOUT;
	u32 status  = 0;

	/* Set address */
	dev_write(dev, PHY_I2CM_ADDRESS, addr);

	dev_write(dev, PHY_I2CM_OPERATION, PHY_I2CM_OPERATION_RD_MASK);

	do {
		snps_sleep(10);
		status = dev_read_mask(dev, IH_I2CMPHY_STAT0,
				IH_I2CMPHY_STAT0_I2CMPHYERROR_MASK |
				IH_I2CMPHY_STAT0_I2CMPHYDONE_MASK);
	} while (status == 0 && (timeout--));

	dev_write(dev, IH_I2CMPHY_STAT0, status); /* clear read status */

	if (status & IH_I2CMPHY_STAT0_I2CMPHYERROR_MASK) {
		pr_info(" I2C Read failed\n");
		return -1;
	}

	if (status & IH_I2CMPHY_STAT0_I2CMPHYDONE_MASK) {

		*value = ((u16) (dev_read(dev, (PHY_I2CM_DATAI_1)) << 8)
				| dev_read(dev, (PHY_I2CM_DATAI_0)));
		return 0;
	}

	pr_info(" ASSERT I2C Read timeout - check PHY - exiting\n");
	return -1;
}

#ifdef SUPPORT_PHY_JTAG
static void _send_data_pulse(hdmi_tx_dev_t *dev, u8 tms, u8 tdi)
{
	u8 in_value = 0;

	in_value = set(in_value, JTAG_PHY_TAP_IN_JTAG_TMS_MASK, tms);
	in_value = set(in_value, JTAG_PHY_TAP_IN_JTAG_TDI_MASK, tdi);

	dev_write(dev, JTAG_PHY_TAP_TCK, (u8) 0);
	snps_sleep(100);
	dev_write(dev, JTAG_PHY_TAP_IN, in_value);
	snps_sleep(100);
	dev_write(dev, JTAG_PHY_TAP_TCK, (u8) 1);
	snps_sleep(100);
}

static void _tap_soft_reset(hdmi_tx_dev_t *dev)
{
	int i;

	for (i = 0; i < 5; i++)
		_send_data_pulse(dev, 1, 0);

	_send_data_pulse(dev, 0, 0);
}

static void _tap_goto_shift_dr(hdmi_tx_dev_t *dev)
{
	/* RTI -> Select-DR */
	_send_data_pulse(dev, 1, 0);
	/* Select-DR-Scan -> Capture-DR -> Shift-DR */
	_send_data_pulse(dev, 0, 0);
	_send_data_pulse(dev, 0, 0);
}

static void _tap_goto_shift_ir(hdmi_tx_dev_t *dev)
{
	/* RTI->Sel IR_Scan */
	_send_data_pulse(dev, 1, 0);
	_send_data_pulse(dev, 1, 0);
	/* Select-IR-Scan -> Shift_IR */
	_send_data_pulse(dev, 0, 0);
	_send_data_pulse(dev, 0, 0);
}


static void _tap_goto_run_test_idle(hdmi_tx_dev_t *dev)
{
	/* Exit1_DR -> Update_DR */
	_send_data_pulse(dev, 1, 0);

	/* Update_DR -> Run_Test_Idle */
	_send_data_pulse(dev, 0, 0);
}

static void _send_value_shift_ir(hdmi_tx_dev_t *dev, u8 jtag_addr)
{
	int i;

	for (i = 0; i < 7; i++) {
		_send_data_pulse(dev, 0, jtag_addr & 0x01);
		jtag_addr = jtag_addr >> 1;
	}
	/* Shift_IR -> Exit_IR w/ last MSB bit */
	_send_data_pulse(dev, 1, jtag_addr & 0x01);
}

static u16 _send_value_shift_dr(hdmi_tx_dev_t *dev, u8 cmd, u16 data_in)
{
	int i;
	u32 aux_in = (cmd << 16) | data_in;
	u16 data_out = 0;
	/* Shift_DR */
	for (i = 0; i < 16; i++) {
		_send_data_pulse(dev, 0, aux_in);
		data_out |= (dev_read(dev, JTAG_PHY_TAP_OUT) & 0x01) << i;
		aux_in = aux_in >> 1;
	}
	/* Shift_DR, TAP command bit */
	_send_data_pulse(dev, 0, aux_in);
	aux_in = aux_in >> 1;

	/* Shift_DR -> Exit_DR w/ MSB TAP command bit */
	i++;
	_send_data_pulse(dev, 1, aux_in);
	data_out |= (dev_read(dev, JTAG_PHY_TAP_OUT) & 0x01) << i;

	return data_out;
}

static int phy_jtag_init(hdmi_tx_dev_t *dev, u8 jtag_addr)
{
	dev_write(dev, JTAG_PHY_ADDR, jtag_addr);
	phy_jtag_reset(dev);
	_tap_soft_reset(dev);
	phy_jtag_slave_address(dev, jtag_addr);

	return 1;
}

static void phy_jtag_slave_address(hdmi_tx_dev_t *dev, u8 jtag_addr)
{
	_tap_goto_shift_ir(dev);

	/* Shift-IR - write jtag slave address */
	_send_value_shift_ir(dev, jtag_addr);

	_tap_goto_run_test_idle(dev);

}

static int phy_jtag_read(hdmi_tx_dev_t *dev, u16 addr,  u16 *pvalue)
{
	_tap_goto_shift_dr(dev);

	/* Shift-DR (shift 16 times) and -> Exit1 -DR */
	_send_value_shift_dr(dev, JTAG_TAP_ADDR_CMD, addr << 8);

	_tap_goto_run_test_idle(dev);
	_tap_goto_shift_dr(dev);

	*pvalue = _send_value_shift_dr(dev, JTAG_TAP_READ_CMD, 0xFFFF);

	_tap_goto_run_test_idle(dev);

	return 0;
}

static int phy_jtag_write(hdmi_tx_dev_t *dev, u16 addr,  u16 value)
{
	_tap_goto_shift_dr(dev);

	/* Shift-DR (shift 16 times) and -> Exit1 -DR */
	_send_value_shift_dr(dev, JTAG_TAP_ADDR_CMD, addr << 8);

	_tap_goto_run_test_idle(dev);
	_tap_goto_shift_dr(dev);

	_send_value_shift_dr(dev, JTAG_TAP_WRITE_CMD, value);
	_tap_goto_run_test_idle(dev);

	return 0;
}

static void phy_jtag_reset(hdmi_tx_dev_t *dev)
{
	dev_write(dev, JTAG_PHY_TAP_IN, 0x10);
	snps_sleep(100);
	dev_write(dev, JTAG_PHY_CONFIG, 0);
	snps_sleep(100);
	dev_write(dev, JTAG_PHY_CONFIG, 1); /* enable interface to JTAG */
	_send_data_pulse(dev, 0, 0);
}
#endif

static int phy_write(hdmi_tx_dev_t *dev, u8 addr, u16 data)
{
	switch (dev->snps_hdmi_ctrl.phy_access) {
#ifdef SUPPORT_PHY_JTAG
	case PHY_JTAG:
		return phy_jtag_write(dev, addr, data);
#endif
	case PHY_I2C:
		return phy_i2c_write(dev, addr, data);
	default:
		pr_err("PHY interface not defined");
	}
	return -1;
}

static int phy_read(hdmi_tx_dev_t *dev, u8 addr, u16 *value)
{
	switch (dev->snps_hdmi_ctrl.phy_access) {
#ifdef SUPPORT_PHY_JTAG
	case PHY_JTAG:
		return phy_jtag_read(dev, addr, value);
#endif
	case PHY_I2C:
		return phy_i2c_read(dev, addr, value);
	default:
		pr_err("PHY interface not defined");
	}
	return -1;
}


/* phy301 clock: unit:kHZ */
static struct phy_config phy301[] = {
	{13500, PIXEL_REPETITION_1, COLOR_DEPTH_8, HDMI_14, 0x133, 0x0, 0x0, 0x4, 0x232, 0x8009},
	{13500, PIXEL_REPETITION_1, COLOR_DEPTH_10, HDMI_14, 0x2173, 0x0, 0x0, 0x4, 0x232, 0x8009},
	{13500, PIXEL_REPETITION_1, COLOR_DEPTH_12, HDMI_14, 0x41B3, 0x0, 0x0, 0x4, 0x232, 0x8009},
	{13500, PIXEL_REPETITION_1, COLOR_DEPTH_16, HDMI_14, 0x6132, 0x8, 0x1, 0x4, 0x232, 0x8009},
	{13500, PIXEL_REPETITION_3, COLOR_DEPTH_8, HDMI_14, 0x132, 0x8, 0x1, 0x4, 0x232, 0x8009},
	{13500, PIXEL_REPETITION_3, COLOR_DEPTH_10, HDMI_14, 0x2172, 0x8, 0x1, 0x4, 0x232, 0x8009},
	{13500, PIXEL_REPETITION_3, COLOR_DEPTH_12, HDMI_14, 0x41B2, 0x8, 0x1, 0x4, 0x232, 0x8009},
	{13500, PIXEL_REPETITION_3, COLOR_DEPTH_16, HDMI_14, 0x6131, 0x18, 0x2, 0x4, 0x232, 0x8009},
	{13500, PIXEL_REPETITION_7, COLOR_DEPTH_8, HDMI_14, 0x131, 0x18, 0x2, 0x4, 0x232, 0x8009},
	{13500, PIXEL_REPETITION_7, COLOR_DEPTH_10, HDMI_14, 0x2171, 0x18, 0x2, 0x4, 0x232, 0x8009},
	{13500, PIXEL_REPETITION_7, COLOR_DEPTH_12, HDMI_14, 0x41B1, 0x18, 0x2, 0x4, 0x232, 0x8009},
	{13500, PIXEL_REPETITION_7, COLOR_DEPTH_16, HDMI_14, 0x6130, 0x30, 0x3, 0x4, 0x232, 0x8009},
	{18000, PIXEL_REPETITION_2, COLOR_DEPTH_8, HDMI_14, 0x00F2, 0x8, 0x1, 0x4, 0x232, 0x8009},
	{18000, PIXEL_REPETITION_2, COLOR_DEPTH_10, HDMI_14, 0x2162, 0x8, 0x1, 0x4, 0x232, 0x8009},
	{18000, PIXEL_REPETITION_2, COLOR_DEPTH_12, HDMI_14, 0x41A2, 0x8, 0x1, 0x4, 0x232, 0x8009},
	{18000, PIXEL_REPETITION_2, COLOR_DEPTH_16, HDMI_14, 0x60F1, 0x18, 0x2, 0x4, 0x232, 0x8009},
	{18000, PIXEL_REPETITION_5, COLOR_DEPTH_8, HDMI_14, 0x00F1, 0x18, 0x2, 0x4, 0x232, 0x8009},
	{18000, PIXEL_REPETITION_5, COLOR_DEPTH_10, HDMI_14, 0x2161, 0x18, 0x2, 0x4, 0x232, 0x8009},
	{18000, PIXEL_REPETITION_5, COLOR_DEPTH_12, HDMI_14, 0x41A1, 0x18, 0x2, 0x4, 0x232, 0x8009},
	{18000, PIXEL_REPETITION_5, COLOR_DEPTH_16, HDMI_14, 0x60F0, 0x31, 0x3, 0x4, 0x232, 0x8009},
	{21600, PIXEL_REPETITION_4, COLOR_DEPTH_8, HDMI_14, 0x151, 0x18, 0x2, 0x4, 0x232, 0x8009},
	{21600, PIXEL_REPETITION_4, COLOR_DEPTH_12, HDMI_14, 0x4161, 0x18, 0x2, 0x4, 0x232, 0x8009},
	{21600, PIXEL_REPETITION_4, COLOR_DEPTH_16, HDMI_14, 0x6150, 0x31, 0x3, 0x4, 0x232, 0x8009},
	{21600, PIXEL_REPETITION_9, COLOR_DEPTH_8, HDMI_14, 0x150, 0x31, 0x3, 0x4, 0x232, 0x8009},
	{21600, PIXEL_REPETITION_9, COLOR_DEPTH_12, HDMI_14, 0x4160, 0x30, 0x3, 0x4, 0x232, 0x8009},
	{21600, PIXEL_REPETITION_9, COLOR_DEPTH_16, HDMI_20, 0x7B70, 0x001B, 0x3, 0x4, 0x14A, 0x8039},
	{24000, PIXEL_REPETITION_8, COLOR_DEPTH_8, HDMI_14, 0x00E0, 0x32, 0x3, 0x4, 0x232, 0x8009},
	{24000, PIXEL_REPETITION_8, COLOR_DEPTH_16, HDMI_20, 0x7BA0, 0x001B, 0x3, 0x4, 0x14A, 0x8039},
	{25175, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x00B3, 0x0, 0x0, 0x4, 0x232, 0x8009},
	{25175, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, HDMI_14, 0x2153, 0x0, 0x0, 0x4, 0x232, 0x8009},
	{25175, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, HDMI_14, 0x40F3, 0x0, 0x0, 0x4, 0x232, 0x8009},
	{25175, PIXEL_REPETITION_OFF, COLOR_DEPTH_16, HDMI_14, 0x60B2, 0x8, 0x1, 0x4, 0x232, 0x8009},
#ifdef CONFIG_ARCH_SUN8IW16
	{27000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x00B3, 0x12, 0x0, 0x7, 0x2b0, 0x8039},
#else
	{27000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x00B3, 0x12, 0x0, 0x7, 0x2b0, 0x8009},
#endif
	{27000, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, HDMI_14, 0x2153, 0x0, 0x0, 0x4, 0x2b0, 0x8009},
	{27000, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, HDMI_14, 0x40F3, 0x0, 0x0, 0x4, 0x232, 0x8009},
	{27000, PIXEL_REPETITION_OFF, COLOR_DEPTH_16, HDMI_14, 0x60B2, 0x8, 0x1, 0x4, 0x232, 0x8009},
	{27000, PIXEL_REPETITION_1, COLOR_DEPTH_8, HDMI_14, 0x00B2, 0x8, 0x1, 0x4, 0x232, 0x8009},
	{27000, PIXEL_REPETITION_1, COLOR_DEPTH_10, HDMI_14, 0x2152, 0x8, 0x1, 0x4, 0x232, 0x8009},
	{27000, PIXEL_REPETITION_1, COLOR_DEPTH_12, HDMI_14, 0x40F2, 0x8, 0x1, 0x4, 0x232, 0x8009},
	{27000, PIXEL_REPETITION_1, COLOR_DEPTH_16, HDMI_14, 0x60B1, 0x19, 0x2, 0x4, 0x232, 0x8009},
	{27000, PIXEL_REPETITION_3, COLOR_DEPTH_8, HDMI_14, 0x00B1, 0x19, 0x2, 0x4, 0x232, 0x8009},
	{27000, PIXEL_REPETITION_3, COLOR_DEPTH_10, HDMI_14, 0x2151, 0x18, 0x2, 0x4, 0x232, 0x8009},
	{27000, PIXEL_REPETITION_3, COLOR_DEPTH_12, HDMI_14, 0x40F1, 0x18, 0x2, 0x4, 0x232, 0x8009},
	{27000, PIXEL_REPETITION_3, COLOR_DEPTH_16, HDMI_14, 0x60B0, 0x32, 0x3, 0x4, 0x232, 0x8009},
	{27000, PIXEL_REPETITION_7, COLOR_DEPTH_8, HDMI_14, 0x00B0, 0x32, 0x3, 0x4, 0x232, 0x8009},
	{27000, PIXEL_REPETITION_7, COLOR_DEPTH_10, HDMI_14, 0x2150, 0x31, 0x3, 0x4, 0x232, 0x8009},
	{27000, PIXEL_REPETITION_7, COLOR_DEPTH_12, HDMI_14, 0x40F0, 0x31, 0x3, 0x4, 0x232, 0x8009},
	{27000, PIXEL_REPETITION_7, COLOR_DEPTH_16, HDMI_20, 0x7B30, 0x001B, 0x3, 0x4, 0x14A, 0x8039},
	{31500, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x00B3, 0x0, 0x0, 0x4, 0x232, 0x8009},
	{33750, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x00B3, 0x0, 0x0, 0x4, 0x232, 0x8009},
	{35500, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x00B3, 0x0, 0x0, 0x4, 0x232, 0x8009},
	{36000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x00B3, 0x0, 0x0, 0x4, 0x232, 0x8009},
	{36000, PIXEL_REPETITION_2, COLOR_DEPTH_8, HDMI_14, 0x00A1, 0x001A, 0x2, 0x4, 0x232, 0x8009},
	{36000, PIXEL_REPETITION_2, COLOR_DEPTH_10, HDMI_14, 0x2165, 0x18, 0x2, 0x4, 0x232, 0x8009},
	{36000, PIXEL_REPETITION_2, COLOR_DEPTH_12, HDMI_14, 0x40E1, 0x19, 0x2, 0x4, 0x232, 0x8009},
	{36000, PIXEL_REPETITION_2, COLOR_DEPTH_16, HDMI_14, 0x60A0, 0x32, 0x3, 0x4, 0x232, 0x8009},
	{36000, PIXEL_REPETITION_5, COLOR_DEPTH_8, HDMI_14, 0x00A0, 0x32, 0x3, 0x4, 0x232, 0x8009},
	{36000, PIXEL_REPETITION_5, COLOR_DEPTH_10, HDMI_14, 0x2164, 0x30, 0x3, 0x4, 0x232, 0x8009},
	{36000, PIXEL_REPETITION_5, COLOR_DEPTH_12, HDMI_14, 0x40E0, 0x32, 0x3, 0x4, 0x232, 0x8009},
	{36000, PIXEL_REPETITION_5, COLOR_DEPTH_16, HDMI_20, 0x7AF0, 0x001B, 0x3, 0x4, 0x14A, 0x8039},
	{40000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x00B3, 0x0, 0x0, 0x4, 0x232, 0x8009},
	{43200, PIXEL_REPETITION_4, COLOR_DEPTH_8, HDMI_14, 0x140, 0x33, 0x3, 0x4, 0x232, 0x8009},
	{43200, PIXEL_REPETITION_4, COLOR_DEPTH_12, HDMI_14, 0x4164, 0x30, 0x3, 0x4, 0x232, 0x8009},
	{43200, PIXEL_REPETITION_4, COLOR_DEPTH_16, HDMI_20, 0x7B50, 0x001B, 0x3, 0x4, 0x14A, 0x8039},
	{44900, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x00B3, 0x0, 0x0, 0x4, 0x232, 0x8009},
	{49500, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x72, 0x8, 0x1, 0x4, 0x232, 0x8009},
	{50000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x72, 0x8, 0x1, 0x4, 0x232, 0x8009},
	{50350, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x72, 0x8, 0x1, 0x4, 0x232, 0x8009},
	{50350, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, HDMI_14, 0x2142, 0x8, 0x1, 0x4, 0x232, 0x8009},
	{50350, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, HDMI_14, 0x40A2, 0x8, 0x1, 0x4, 0x232, 0x8009},
	{50350, PIXEL_REPETITION_OFF, COLOR_DEPTH_16, HDMI_14, 0x6071, 0x001A, 0x2, 0x4, 0x232, 0x8009},
	{54000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x72, 0x8, 0x1, 0x4, 0x232, 0x8009},
	{54000, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, HDMI_14, 0x2142, 0x8, 0x1, 0x4, 0x232, 0x8009},
	{54000, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, HDMI_14, 0x40A2, 0x8, 0x1, 0x4, 0x232, 0x8009},
	{54000, PIXEL_REPETITION_OFF, COLOR_DEPTH_16, HDMI_14, 0x6071, 0x001A, 0x2, 0x4, 0x232, 0x8009},
	{54000, PIXEL_REPETITION_1, COLOR_DEPTH_8, HDMI_14, 0x71, 0x001A, 0x2, 0x4, 0x232, 0x8009},
	{54000, PIXEL_REPETITION_1, COLOR_DEPTH_10, HDMI_14, 0x2141, 0x001A, 0x2, 0x4, 0x232, 0x8009},
	{54000, PIXEL_REPETITION_1, COLOR_DEPTH_12, HDMI_14, 0x40A1, 0x001A, 0x2, 0x4, 0x232, 0x8009},
	{54000, PIXEL_REPETITION_1, COLOR_DEPTH_16, HDMI_14, 0x6070, 0x33, 0x3, 0x4, 0x232, 0x8009},
	{54000, PIXEL_REPETITION_3, COLOR_DEPTH_8, HDMI_14, 0x70, 0x33, 0x3, 0x4, 0x232, 0x8009},
	{54000, PIXEL_REPETITION_3, COLOR_DEPTH_10, HDMI_14, 0x2140, 0x33, 0x3, 0x4, 0x232, 0x8009},
	{54000, PIXEL_REPETITION_3, COLOR_DEPTH_12, HDMI_14, 0x40A0, 0x32, 0x3, 0x4, 0x232, 0x8009},
	{54000, PIXEL_REPETITION_3, COLOR_DEPTH_16, HDMI_20, 0x7AB0, 0x001B, 0x3, 0x4, 0x14A, 0x8039},
	{56250, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x72, 0x8, 0x1, 0x4, 0x232, 0x8009},
	{59400, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x72, 0x8, 0x1, 0x4, 0x232, 0x8009},
	{59400, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, HDMI_14, 0x2142, 0x8, 0x1, 0x4, 0x232, 0x8009},
	{59400, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, HDMI_14, 0x40A2, 0x8, 0x1, 0x4, 0x232, 0x8009},
	{59400, PIXEL_REPETITION_OFF, COLOR_DEPTH_16, HDMI_14, 0x6071, 0x001A, 0x2, 0x4, 0x232, 0x8009},
	{65000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x72, 0x8, 0x1, 0x4, 0x232, 0x8009},
	{68250, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x72, 0x8, 0x1, 0x4, 0x232, 0x8009},
	{71000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x72, 0x8, 0x1, 0x4, 0x232, 0x8009},
	{72000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x72, 0x8, 0x1, 0x4, 0x232, 0x8009},
	{72000, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, HDMI_14, 0x2142, 0x8, 0x1, 0x4, 0x232, 0x8009},
	{72000, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, HDMI_14, 0x4061, 0x001B, 0x2, 0x4, 0x232, 0x8009},
	{72000, PIXEL_REPETITION_OFF, COLOR_DEPTH_16, HDMI_14, 0x6071, 0x001A, 0x2, 0x4, 0x232, 0x8009},
	{72000, PIXEL_REPETITION_2, COLOR_DEPTH_8, HDMI_14, 0x60, 0x34, 0x3, 0x4, 0x232, 0x8009},
	{72000, PIXEL_REPETITION_2, COLOR_DEPTH_10, HDMI_14, 0x216C, 0x30, 0x3, 0x4, 0x232, 0x8009},
	{72000, PIXEL_REPETITION_2, COLOR_DEPTH_12, HDMI_14, 0x40E4, 0x32, 0x3, 0x4, 0x232, 0x8009},
	{72000, PIXEL_REPETITION_2, COLOR_DEPTH_16, HDMI_20, 0x7AA0, 0x001B, 0x3, 0x4, 0x14A, 0x8039},
	{73250, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x72, 0x8, 0x1, 0x4, 0x232, 0x8009},
#ifdef CONFIG_ARCH_SUN8IW16
	{74250, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x72, 0x13, 0x1, 0x6, 0x2b0, 0x8039},
#elif IS_ENABLED(CONFIG_ARCH_SUN50IW9)
	{74250, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x72, 0x13, 0x1, 0x4, 0x290, 0x8019},
#else
	{74250, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x72, 0x13, 0x1, 0x6, 0x22d, 0x8009},
#endif
	{74250, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, HDMI_14, 0x2145, 0x0013, 0x1, 0x6, 0x270, 0x8029},
	{74250, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, HDMI_14, 0x4061, 0x0013, 0x1, 0x6, 0x270, 0x8029},
	{74250, PIXEL_REPETITION_OFF, COLOR_DEPTH_16, HDMI_14, 0x6071, 0x001A, 0x2, 0x4, 0x230, 0x8009},
	{75000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x72, 0x8, 0x1, 0x4, 0x232, 0x8009},
	{78750, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x72, 0x8, 0x1, 0x4, 0x232, 0x8009},
	{79500, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x72, 0x8, 0x1, 0x4, 0x232, 0x8009},
	{82500, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x72, 0x8, 0x1, 0x4, 0x232, 0x8009},
	{82500, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, HDMI_14, 0x2145, 0x001A, 0x2, 0x4, 0x232, 0x8009},
	{82500, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, HDMI_14, 0x4061, 0x001B, 0x2, 0x4, 0x232, 0x8009},
	{82500, PIXEL_REPETITION_OFF, COLOR_DEPTH_16, HDMI_14, 0x6071, 0x001A, 0x2, 0x4, 0x230, 0x8009},
	{83500, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x72, 0x8, 0x1, 0x4, 0x232, 0x8009},
	{85500, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x72, 0x8, 0x1, 0x4, 0x232, 0x8009},
	{88750, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x72, 0x8, 0x1, 0x4, 0x232, 0x8009},
	{90000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x72, 0x8, 0x1, 0x4, 0x232, 0x8009},
	{90000, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, HDMI_14, 0x2145, 0x001A, 0x2, 0x4, 0x232, 0x8009},
	{90000, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, HDMI_14, 0x4061, 0x001B, 0x2, 0x4, 0x232, 0x8009},
	{90000, PIXEL_REPETITION_OFF, COLOR_DEPTH_16, HDMI_14, 0x6071, 0x001A, 0x2, 0x4, 0x230, 0x8009},
	{94500, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x51, 0x001B, 0x2, 0x4, 0x232, 0x8009},
	{99000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x51, 0x001B, 0x2, 0x4, 0x232, 0x8009},
	{99000, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, HDMI_14, 0x2145, 0x001A, 0x2, 0x4, 0x232, 0x8009},
	{99000, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, HDMI_14, 0x4061, 0x001B, 0x2, 0x4, 0x230, 0x8009},
	{99000, PIXEL_REPETITION_OFF, COLOR_DEPTH_16, HDMI_14, 0x6050, 0x35, 0x3, 0x4, 0x230, 0x8009},
	{100700, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x51, 0x001B, 0x2, 0x4, 0x232, 0x8009},
	{100700, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, HDMI_14, 0x2145, 0x001A, 0x2, 0x4, 0x232, 0x8009},
	{100700, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, HDMI_14, 0x4061, 0x001B, 0x2, 0x4, 0x230, 0x8009},
	{100700, PIXEL_REPETITION_OFF, COLOR_DEPTH_16, HDMI_14, 0x6050, 0x35, 0x3, 0x4, 0x230, 0x8009},
	{101000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x51, 0x001B, 0x2, 0x4, 0x232, 0x8009},
	{102250, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x51, 0x001B, 0x2, 0x4, 0x232, 0x8009},
	{106500, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x51, 0x001B, 0x2, 0x4, 0x232, 0x8009},
	{108000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x51, 0x001B, 0x2, 0x4, 0x232, 0x8009},
	{108000, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, HDMI_14, 0x2145, 0x001A, 0x2, 0x4, 0x232, 0x8009},
	{108000, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, HDMI_14, 0x4061, 0x001B, 0x2, 0x4, 0x230, 0x8009},
	{108000, PIXEL_REPETITION_OFF, COLOR_DEPTH_16, HDMI_14, 0x6050, 0x35, 0x3, 0x4, 0x230, 0x8009},
	{108000, PIXEL_REPETITION_1, COLOR_DEPTH_8, HDMI_14, 0x50, 0x35, 0x3, 0x4, 0x232, 0x8009},
	{108000, PIXEL_REPETITION_1, COLOR_DEPTH_10, HDMI_14, 0x2144, 0x33, 0x3, 0x4, 0x232, 0x8009},
	{108000, PIXEL_REPETITION_1, COLOR_DEPTH_12, HDMI_14, 0x4060, 0x34, 0x3, 0x4, 0x230, 0x8009},
	{108000, PIXEL_REPETITION_1, COLOR_DEPTH_16, HDMI_20, 0x7A70, 0x33, 0x3, 0x4, 0x14A, 0x8039},
	{115500, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x51, 0x001B, 0x2, 0x4, 0x232, 0x8009},
	{117500, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x51, 0x001B, 0x2, 0x4, 0x232, 0x8009},
	{118800, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x51, 0x001B, 0x2, 0x4, 0x232, 0x8009},
	{118800, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, HDMI_14, 0x2145, 0x001A, 0x2, 0x4, 0x230, 0x8009},
	{118800, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, HDMI_14, 0x4061, 0x001B, 0x2, 0x4, 0x230, 0x8009},
	{118800, PIXEL_REPETITION_OFF, COLOR_DEPTH_16, HDMI_14, 0x6050, 0x35, 0x3, 0x4, 0x273, 0x8009},
	{119000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x51, 0x001B, 0x2, 0x4, 0x232, 0x8009},
	{121750, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x51, 0x001B, 0x2, 0x4, 0x232, 0x8009},
	{122500, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x51, 0x001B, 0x2, 0x4, 0x232, 0x8009},
	{135000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x51, 0x001B, 0x2, 0x4, 0x232, 0x8009},
	{136750, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x51, 0x001B, 0x2, 0x4, 0x232, 0x8009},
	{140250, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x51, 0x001B, 0x2, 0x4, 0x232, 0x8009},
	{144000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x51, 0x001B, 0x2, 0x4, 0x232, 0x8009},
	{144000, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, HDMI_14, 0x2145, 0x001A, 0x2, 0x4, 0x230, 0x8009},
	{144000, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, HDMI_14, 0x4064, 0x34, 0x3, 0x4, 0x230, 0x8009},
	{144000, PIXEL_REPETITION_OFF, COLOR_DEPTH_16, HDMI_14, 0x6050, 0x35, 0x3, 0x4, 0x273, 0x8009},
	{146250, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x51, 0x001B, 0x2, 0x4, 0x232, 0x8009},
	{148250, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x51, 0x001B, 0x2, 0x4, 0x232, 0x8009},
#ifdef CONFIG_ARCH_SUN8IW16
	{148500, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x51, 0x0019, 0x2, 0x6, 0x2b0, 0x8039},
#elif IS_ENABLED(CONFIG_ARCH_SUN50IW9)
	{148500, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x51, 0x0019, 0x2, 0x4, 0x290, 0x8019},
#else
	{148500, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x51, 0x0019, 0x2, 0x6, 0x270, 0x8029},
#endif
	{148500, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, HDMI_14, 0x214C, 0x19, 0x2, 0x5, 0x1ab, 0x8039},
	{148500, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, HDMI_14, 0x4064, 0x19, 0x2, 0x5, 0x1ab, 0x8039},
	{148500, PIXEL_REPETITION_OFF, COLOR_DEPTH_16, HDMI_14, 0x6050, 0x35, 0x3, 0x4, 0x273, 0x8029},
	{154000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x51, 0x001B, 0x2, 0x4, 0x230, 0x8009},
	{156000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x51, 0x001B, 0x2, 0x4, 0x230, 0x8009},
	{157000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x51, 0x001B, 0x2, 0x4, 0x230, 0x8009},
	{157500, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x51, 0x001B, 0x2, 0x4, 0x230, 0x8009},
	{162000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x51, 0x001B, 0x2, 0x4, 0x230, 0x8009},
	{165000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x51, 0x001B, 0x2, 0x4, 0x230, 0x8009},
	{165000, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, HDMI_14, 0x214C, 0x33, 0x3, 0x4, 0x230, 0x8009},
	{165000, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, HDMI_14, 0x4064, 0x34, 0x3, 0x4, 0x273, 0x8009},
	{165000, PIXEL_REPETITION_OFF, COLOR_DEPTH_16, HDMI_14, 0x6050, 0x35, 0x3, 0x4, 0x273, 0x8029},
	{175500, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x51, 0x001B, 0x2, 0x4, 0x230, 0x8009},
	{179500, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x51, 0x001B, 0x2, 0x4, 0x230, 0x8009},
	{180000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x51, 0x001B, 0x2, 0x4, 0x230, 0x8009},
	{180000, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, HDMI_14, 0x214C, 0x33, 0x3, 0x4, 0x273, 0x8009},
	{180000, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, HDMI_14, 0x4064, 0x34, 0x3, 0x4, 0x273, 0x8009},
	{180000, PIXEL_REPETITION_OFF, COLOR_DEPTH_16, HDMI_20, 0x7A50, 0x001B, 0x3, 0x4, 0x14A, 0x8039},
	{182750, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x51, 0x001B, 0x2, 0x4, 0x230, 0x8009},
	{185625, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x40, 0x36, 0x3, 0x4, 0x230, 0x8009},
	{185625, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, HDMI_14, 0x214C, 0x33, 0x3, 0x4, 0x273, 0x8009},
	{185625, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, HDMI_14, 0x4064, 0x34, 0x3, 0x4, 0x273, 0x8009},
	{185625, PIXEL_REPETITION_OFF, COLOR_DEPTH_16, HDMI_20, 0x7A50, 0x001B, 0x3, 0x4, 0x14A, 0x8039},
	{187000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x40, 0x36, 0x3, 0x4, 0x230, 0x8009},
	{187250, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x40, 0x36, 0x3, 0x4, 0x230, 0x8009},
	{189000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x40, 0x36, 0x3, 0x4, 0x230, 0x8009},
	{193250, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x40, 0x36, 0x3, 0x4, 0x230, 0x8009},
	{198000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x40, 0x36, 0x3, 0x4, 0x230, 0x8009},
	{198000, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, HDMI_14, 0x214C, 0x33, 0x3, 0x4, 0x273, 0x8009},
	{198000, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, HDMI_14, 0x4064, 0x34, 0x3, 0x4, 0x273, 0x8029},
	{198000, PIXEL_REPETITION_OFF, COLOR_DEPTH_16, HDMI_20, 0x7A50, 0x001B, 0x3, 0x4, 0x14A, 0x8039},
	{202500, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x40, 0x36, 0x3, 0x4, 0x230, 0x8009},
	{204750, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x40, 0x36, 0x3, 0x4, 0x230, 0x8009},
	{208000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x40, 0x36, 0x3, 0x4, 0x230, 0x8009},
	{214750, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x40, 0x36, 0x3, 0x4, 0x230, 0x8009},
	{216000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x40, 0x36, 0x3, 0x4, 0x230, 0x8009},
	{216000, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, HDMI_14, 0x214C, 0x33, 0x3, 0x4, 0x273, 0x8009},
	{216000, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, HDMI_14, 0x4064, 0x34, 0x3, 0x4, 0x273, 0x8029},
	{216000, PIXEL_REPETITION_OFF, COLOR_DEPTH_16, HDMI_20, 0x7A50, 0x001B, 0x3, 0x4, 0x14A, 0x8039},
	{218250, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x40, 0x36, 0x3, 0x4, 0x230, 0x8009},
	{229500, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x40, 0x36, 0x3, 0x4, 0x273, 0x8009},
	{234000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x40, 0x36, 0x3, 0x4, 0x273, 0x8009},
	{237600, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x40, 0x36, 0x3, 0x4, 0x273, 0x8009},
	{237600, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, HDMI_14, 0x214C, 0x33, 0x3, 0x4, 0x273, 0x8029},
	{237600, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, HDMI_20, 0x5A64, 0x001B, 0x3, 0x4, 0x14A, 0x8039},
	{237600, PIXEL_REPETITION_OFF, COLOR_DEPTH_16, HDMI_20, 0x7A50, 0x001B, 0x3, 0x4, 0x14A, 0x8039},
	{245250, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x40, 0x36, 0x3, 0x4, 0x273, 0x8009},
	{245500, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x40, 0x36, 0x3, 0x4, 0x273, 0x8009},
	{261000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x40, 0x36, 0x3, 0x4, 0x273, 0x8009},
	{268250, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x40, 0x36, 0x3, 0x4, 0x273, 0x8009},
	{268500, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x40, 0x36, 0x3, 0x4, 0x273, 0x8009},
	{281250, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x40, 0x36, 0x3, 0x4, 0x273, 0x8009},
	{288000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x40, 0x36, 0x3, 0x4, 0x273, 0x8009},
	{288000, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, HDMI_20, 0x3B4C, 0x001B, 0x3, 0x4, 0x14A, 0x8039},
	{288000, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, HDMI_20, 0x5A64, 0x001B, 0x3, 0x4, 0x14A, 0x8039},
	{288000, PIXEL_REPETITION_OFF, COLOR_DEPTH_16, HDMI_20, 0x7A50, 0x003D, 0x3, 0x4, 0x14A, 0x8039},
#ifdef CONFIG_ARCH_SUN8IW16
	{297000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x40, 0x19, 0x3, 0x5, 0x210, 0x803d},
#elif IS_ENABLED(CONFIG_ARCH_SUN50IW9)
	{297000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x40, 0x19, 0x3, 0x4, 0x22b, 0x8039},
#else
	{297000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x40, 0x19, 0x3, 0x5, 0x1ab, 0x8039},
#endif
	{297000, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, HDMI_20, 0x3B4C, 0x001B, 0x3, 0x0, 0x16A, 0x8019},
	{297000, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, HDMI_20, 0x5A64, 0x0019, 0x3, 0x0, 0x8A, 0x8029},
	{297000, PIXEL_REPETITION_OFF, COLOR_DEPTH_16, HDMI_20, 0x7A50, 0x003D, 0x3, 0x4, 0x14A, 0x8039},
	{317000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x40, 0x36, 0x3, 0x4, 0x273, 0x8029},
	{330000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x40, 0x36, 0x3, 0x4, 0x273, 0x8029},
	{330000, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, HDMI_20, 0x3B4C, 0x001B, 0x3, 0x4, 0x14A, 0x8039},
	{330000, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, HDMI_20, 0x5A64, 0x001B, 0x3, 0x4, 0x14A, 0x8039},
	{333250, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x40, 0x36, 0x3, 0x4, 0x273, 0x8029},
	{340000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x40, 0x36, 0x3, 0x4, 0x273, 0x8029},
	{348500, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_20, 0x1A40, 0x003F, 0x3, 0x4, 0x14A, 0x8039},
	{356500, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_20, 0x1A40, 0x003F, 0x3, 0x4, 0x14A, 0x8039},
	{360000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_20, 0x1A40, 0x003F, 0x3, 0x4, 0x14A, 0x8039},
	{360000, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, HDMI_20, 0x3B4C, 0x001B, 0x3, 0x4, 0x14A, 0x8039},
	{360000, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, HDMI_20, 0x5A64, 0x001B, 0x3, 0x4, 0x14A, 0x8039},
	{371250, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_20, 0x1A40, 0x003F, 0x3, 0x4, 0x14A, 0x8039},
	{371250, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, HDMI_20, 0x3B4C, 0x001B, 0x3, 0x4, 0x14A, 0x8039},
	{371250, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, HDMI_20, 0x5A64, 0x001B, 0x3, 0x4, 0x14A, 0x8039},
	{396000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_20, 0x1A40, 0x003F, 0x3, 0x4, 0x14A, 0x8039},
	{396000, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, HDMI_20, 0x3B4C, 0x001B, 0x3, 0x4, 0x14A, 0x8039},
	{396000, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, HDMI_20, 0x5A64, 0x001B, 0x3, 0x4, 0x14A, 0x8039},
	{432000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_20, 0x1A40, 0x003F, 0x3, 0x4, 0x14A, 0x8039},
	{432000, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, HDMI_20, 0x3B4C, 0x001B, 0x3, 0x4, 0x14A, 0x8039},
	{443250, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_20, 0x1A40, 0x003F, 0x3, 0x4, 0x14A, 0x8039},
	{475200, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_20, 0x1A40, 0x003F, 0x3, 0x4, 0x14A, 0x8039},
	{475200, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, HDMI_20, 0x3B4C, 0x001B, 0x3, 0x4, 0x14A, 0x8039},
	{495000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_20, 0x1A40, 0x003F, 0x3, 0x4, 0x14A, 0x8039},
	{505250, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_20, 0x1A40, 0x003F, 0x3, 0x4, 0x14A, 0x8039},
	{552750, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_20, 0x1A40, 0x003F, 0x3, 0x4, 0x14A, 0x8039},
	{594000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_20, 0x1A7c, 0x0010, 0x3, 0x0, 0x8A, 0x8029},
	{0, PIXEL_REPETITION_OFF, COLOR_DEPTH_INVALID, HDMI_14, 0, 0, 0, 0, 0, 0}
};

#ifdef SUPPORT_PHY303
static struct phy_config303 phy303[] = {
	{13500, PIXEL_REPETITION_1, COLOR_DEPTH_8, HDMI_14, 0x133, 0x18, 0x0, 0x4, 0x232, 0x8009},
	{13500, PIXEL_REPETITION_1, COLOR_DEPTH_10, HDMI_14, 0x2173, 0x18, 0x0, 0x4, 0x232, 0x8009},
	{13500, PIXEL_REPETITION_1, COLOR_DEPTH_12, HDMI_14, 0x41B3, 0x18, 0x0, 0x4, 0x232, 0x8009},
	{13500, PIXEL_REPETITION_1, COLOR_DEPTH_16, HDMI_14, 0x6132, 0x003B, 0x1, 0x4, 0x232, 0x8009},
	{13500, PIXEL_REPETITION_3, COLOR_DEPTH_8, HDMI_14, 0x132, 0x003B, 0x1, 0x4, 0x232, 0x8009},
	{13500, PIXEL_REPETITION_3, COLOR_DEPTH_10, HDMI_14, 0x2172, 0x003B, 0x1, 0x4, 0x232, 0x8009},
	{13500, PIXEL_REPETITION_3, COLOR_DEPTH_12, HDMI_14, 0x41B2, 0x003B, 0x1, 0x4, 0x232, 0x8009},
	{13500, PIXEL_REPETITION_3, COLOR_DEPTH_16, HDMI_14, 0x6131, 0x39, 0x2, 0x4, 0x232, 0x8009},
	{13500, PIXEL_REPETITION_7, COLOR_DEPTH_8, HDMI_14, 0x131, 0x39, 0x2, 0x4, 0x232, 0x8009},
	{13500, PIXEL_REPETITION_7, COLOR_DEPTH_10, HDMI_14, 0x2171, 0x39, 0x2, 0x4, 0x232, 0x8009},
	{13500, PIXEL_REPETITION_7, COLOR_DEPTH_12, HDMI_14, 0x41B1, 0x39, 0x2, 0x4, 0x232, 0x8009},
	{13500, PIXEL_REPETITION_7, COLOR_DEPTH_16, HDMI_14, 0x6130, 0x38, 0x3, 0x4, 0x232, 0x8009},
	{18000, PIXEL_REPETITION_2, COLOR_DEPTH_8, HDMI_14, 0x00F2, 0x003B, 0x1, 0x4, 0x232, 0x8009},
	{18000, PIXEL_REPETITION_2, COLOR_DEPTH_10, HDMI_14, 0x2162, 0x003B, 0x1, 0x4, 0x232, 0x8009},
	{18000, PIXEL_REPETITION_2, COLOR_DEPTH_12, HDMI_14, 0x41A2, 0x003B, 0x1, 0x4, 0x232, 0x8009},
	{18000, PIXEL_REPETITION_2, COLOR_DEPTH_16, HDMI_14, 0x60F1, 0x003B, 0x2, 0x4, 0x232, 0x8009},
	{18000, PIXEL_REPETITION_5, COLOR_DEPTH_8, HDMI_14, 0x00F1, 0x003B, 0x2, 0x4, 0x232, 0x8009},
	{18000, PIXEL_REPETITION_5, COLOR_DEPTH_10, HDMI_14, 0x2161, 0x39, 0x2, 0x4, 0x232, 0x8009},
	{18000, PIXEL_REPETITION_5, COLOR_DEPTH_12, HDMI_14, 0x41A1, 0x39, 0x2, 0x4, 0x232, 0x8009},
	{18000, PIXEL_REPETITION_5, COLOR_DEPTH_16, HDMI_14, 0x60F0, 0x38, 0x3, 0x4, 0x232, 0x8009},
	{21600, PIXEL_REPETITION_4, COLOR_DEPTH_8, HDMI_14, 0x151, 0x003D, 0x2, 0x4, 0x232, 0x8009},
	{21600, PIXEL_REPETITION_4, COLOR_DEPTH_12, HDMI_14, 0x4161, 0x39, 0x2, 0x4, 0x232, 0x8009},
	{21600, PIXEL_REPETITION_4, COLOR_DEPTH_16, HDMI_14, 0x6150, 0x38, 0x3, 0x4, 0x232, 0x8009},
	{21600, PIXEL_REPETITION_9, COLOR_DEPTH_8, HDMI_14, 0x150, 0x38, 0x3, 0x4, 0x232, 0x8009},
	{21600, PIXEL_REPETITION_9, COLOR_DEPTH_12, HDMI_14, 0x4160, 0x38, 0x3, 0x4, 0x232, 0x8009},
	{21600, PIXEL_REPETITION_9, COLOR_DEPTH_16, HDMI_20, 0x7B70, 0x003D, 0x3, 0x4, 0x14A, 0x8039},
	{24000, PIXEL_REPETITION_8, COLOR_DEPTH_8, HDMI_14, 0x00E0, 0x39, 0x3, 0x4, 0x232, 0x8009},
	{24000, PIXEL_REPETITION_8, COLOR_DEPTH_16, HDMI_20, 0x7BA0, 0x003D, 0x3, 0x4, 0x14A, 0x8039},
	{25175, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x00B3, 0x001A, 0x0, 0x4, 0x232, 0x8009},
	{25175, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, HDMI_14, 0x2153, 0x19, 0x0, 0x4, 0x232, 0x8009},
	{25175, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, HDMI_14, 0x40F3, 0x19, 0x0, 0x4, 0x232, 0x8009},
	{25175, PIXEL_REPETITION_OFF, COLOR_DEPTH_16, HDMI_14, 0x60B2, 0x003B, 0x1, 0x4, 0x232, 0x8009},
	{27000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x00B3, 0x001A, 0x0, 0x4, 0x232, 0x8009},
	{27000, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, HDMI_14, 0x2153, 0x19, 0x0, 0x4, 0x232, 0x8009},
	{27000, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, HDMI_14, 0x40F3, 0x19, 0x0, 0x4, 0x232, 0x8009},
	{27000, PIXEL_REPETITION_OFF, COLOR_DEPTH_16, HDMI_14, 0x60B2, 0x003B, 0x1, 0x4, 0x232, 0x8009},
	{27000, PIXEL_REPETITION_1, COLOR_DEPTH_8, HDMI_14, 0x00B2, 0x003B, 0x1, 0x4, 0x232, 0x8009},
	{27000, PIXEL_REPETITION_1, COLOR_DEPTH_10, HDMI_14, 0x2152, 0x003B, 0x1, 0x4, 0x232, 0x8009},
	{27000, PIXEL_REPETITION_1, COLOR_DEPTH_12, HDMI_14, 0x40F2, 0x003B, 0x1, 0x4, 0x232, 0x8009},
	{27000, PIXEL_REPETITION_1, COLOR_DEPTH_16, HDMI_14, 0x60B1, 0x003F, 0x2, 0x4, 0x232, 0x8009},
	{27000, PIXEL_REPETITION_3, COLOR_DEPTH_8, HDMI_14, 0x00B1, 0x003F, 0x2, 0x4, 0x232, 0x8009},
	{27000, PIXEL_REPETITION_3, COLOR_DEPTH_10, HDMI_14, 0x2151, 0x003D, 0x2, 0x4, 0x232, 0x8009},
	{27000, PIXEL_REPETITION_3, COLOR_DEPTH_12, HDMI_14, 0x40F1, 0x003B, 0x2, 0x4, 0x232, 0x8009},
	{27000, PIXEL_REPETITION_3, COLOR_DEPTH_16, HDMI_14, 0x60B0, 0x39, 0x3, 0x4, 0x232, 0x8009},
	{27000, PIXEL_REPETITION_7, COLOR_DEPTH_8, HDMI_14, 0x00B0, 0x39, 0x3, 0x4, 0x232, 0x8009},
	{27000, PIXEL_REPETITION_7, COLOR_DEPTH_10, HDMI_14, 0x2150, 0x38, 0x3, 0x4, 0x232, 0x8009},
	{27000, PIXEL_REPETITION_7, COLOR_DEPTH_12, HDMI_14, 0x40F0, 0x38, 0x3, 0x4, 0x232, 0x8009},
	{27000, PIXEL_REPETITION_7, COLOR_DEPTH_16, HDMI_20, 0x7B30, 0x003D, 0x3, 0x4, 0x14A, 0x8039},
	{31500, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x00B3, 0x001A, 0x0, 0x4, 0x232, 0x8009},
	{33750, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x00B3, 0x001A, 0x0, 0x4, 0x232, 0x8009},
	{35500, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x00B3, 0x001A, 0x0, 0x4, 0x232, 0x8009},
	{36000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x00B3, 0x001A, 0x0, 0x4, 0x232, 0x8009},
	{36000, PIXEL_REPETITION_2, COLOR_DEPTH_8, HDMI_14, 0x00A1, 0x003F, 0x2, 0x4, 0x232, 0x8009},
	{36000, PIXEL_REPETITION_2, COLOR_DEPTH_10, HDMI_14, 0x2165, 0x39, 0x2, 0x4, 0x232, 0x8009},
	{36000, PIXEL_REPETITION_2, COLOR_DEPTH_12, HDMI_14, 0x40E1, 0x003F, 0x2, 0x4, 0x232, 0x8009},
	{36000, PIXEL_REPETITION_2, COLOR_DEPTH_16, HDMI_14, 0x60A0, 0x003B, 0x3, 0x4, 0x232, 0x8009},
	{36000, PIXEL_REPETITION_5, COLOR_DEPTH_8, HDMI_14, 0x00A0, 0x003B, 0x3, 0x4, 0x232, 0x8009},
	{36000, PIXEL_REPETITION_5, COLOR_DEPTH_10, HDMI_14, 0x2164, 0x38, 0x3, 0x4, 0x232, 0x8009},
	{36000, PIXEL_REPETITION_5, COLOR_DEPTH_12, HDMI_14, 0x40E0, 0x39, 0x3, 0x4, 0x232, 0x8009},
	{36000, PIXEL_REPETITION_5, COLOR_DEPTH_16, HDMI_20, 0x7AF0, 0x003D, 0x3, 0x4, 0x14A, 0x8039},
	{40000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x00B3, 0x001A, 0x0, 0x4, 0x232, 0x8009},
	{43200, PIXEL_REPETITION_4, COLOR_DEPTH_8, HDMI_14, 0x140, 0x003D, 0x3, 0x4, 0x232, 0x8009},
	{43200, PIXEL_REPETITION_4, COLOR_DEPTH_12, HDMI_14, 0x4164, 0x38, 0x3, 0x4, 0x232, 0x8009},
	{43200, PIXEL_REPETITION_4, COLOR_DEPTH_16, HDMI_20, 0x7B50, 0x003D, 0x3, 0x4, 0x14A, 0x8039},
	{44900, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x00B3, 0x001A, 0x0, 0x4, 0x232, 0x8009},
	{49500, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x72, 0x001B, 0x1, 0x4, 0x232, 0x8009},
	{50000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x72, 0x001B, 0x1, 0x4, 0x232, 0x8009},
	{50350, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x72, 0x001B, 0x1, 0x4, 0x232, 0x8009},
	{50350, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, HDMI_14, 0x2142, 0x23, 0x1, 0x4, 0x232, 0x8009},
	{50350, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, HDMI_14, 0x40A2, 0x002A, 0x1, 0x4, 0x232, 0x8009},
	{50350, PIXEL_REPETITION_OFF, COLOR_DEPTH_16, HDMI_14, 0x6071, 0x003F, 0x2, 0x4, 0x232, 0x8009},
	{54000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x72, 0x001B, 0x1, 0x4, 0x232, 0x8009},
	{54000, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, HDMI_14, 0x2142, 0x23, 0x1, 0x4, 0x232, 0x8009},
	{54000, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, HDMI_14, 0x40A2, 0x002A, 0x1, 0x4, 0x232, 0x8009},
	{54000, PIXEL_REPETITION_OFF, COLOR_DEPTH_16, HDMI_14, 0x6071, 0x003F, 0x2, 0x4, 0x232, 0x8009},
	{54000, PIXEL_REPETITION_1, COLOR_DEPTH_8, HDMI_14, 0x71, 0x003F, 0x2, 0x4, 0x232, 0x8009},
	{54000, PIXEL_REPETITION_1, COLOR_DEPTH_10, HDMI_14, 0x2141, 0x003F, 0x2, 0x4, 0x232, 0x8009},
	{54000, PIXEL_REPETITION_1, COLOR_DEPTH_12, HDMI_14, 0x40A1, 0x003F, 0x2, 0x4, 0x232, 0x8009},
	{54000, PIXEL_REPETITION_1, COLOR_DEPTH_16, HDMI_14, 0x6070, 0x003F, 0x3, 0x4, 0x232, 0x8009},
	{54000, PIXEL_REPETITION_3, COLOR_DEPTH_8, HDMI_14, 0x70, 0x003F, 0x3, 0x4, 0x232, 0x8009},
	{54000, PIXEL_REPETITION_3, COLOR_DEPTH_10, HDMI_14, 0x2140, 0x003D, 0x3, 0x4, 0x232, 0x8009},
	{54000, PIXEL_REPETITION_3, COLOR_DEPTH_12, HDMI_14, 0x40A0, 0x003B, 0x3, 0x4, 0x232, 0x8009},
	{54000, PIXEL_REPETITION_3, COLOR_DEPTH_16, HDMI_20, 0x7AB0, 0x003D, 0x3, 0x4, 0x14A, 0x8039},
	{56250, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x72, 0x001B, 0x1, 0x4, 0x232, 0x8009},
	{59400, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x72, 0x001B, 0x1, 0x4, 0x232, 0x8009},
	{59400, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, HDMI_14, 0x2142, 0x23, 0x1, 0x4, 0x232, 0x8009},
	{59400, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, HDMI_14, 0x40A2, 0x002A, 0x1, 0x4, 0x232, 0x8009},
	{59400, PIXEL_REPETITION_OFF, COLOR_DEPTH_16, HDMI_14, 0x6071, 0x003F, 0x2, 0x4, 0x232, 0x8009},
	{65000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x72, 0x001B, 0x1, 0x4, 0x232, 0x8009},
	{68250, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x72, 0x001B, 0x1, 0x4, 0x232, 0x8009},
	{71000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x72, 0x001B, 0x1, 0x4, 0x232, 0x8009},
	{72000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x72, 0x001B, 0x1, 0x4, 0x232, 0x8009},
	{72000, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, HDMI_14, 0x2142, 0x23, 0x1, 0x4, 0x232, 0x8009},
	{72000, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, HDMI_14, 0x4061, 0x002E, 0x2, 0x4, 0x232, 0x8009},
	{72000, PIXEL_REPETITION_OFF, COLOR_DEPTH_16, HDMI_14, 0x6071, 0x003F, 0x2, 0x4, 0x232, 0x8009},
	{72000, PIXEL_REPETITION_2, COLOR_DEPTH_8, HDMI_14, 0x60, 0x003F, 0x3, 0x4, 0x232, 0x8009},
	{72000, PIXEL_REPETITION_2, COLOR_DEPTH_10, HDMI_14, 0x216C, 0x38, 0x3, 0x4, 0x232, 0x8009},
	{72000, PIXEL_REPETITION_2, COLOR_DEPTH_12, HDMI_14, 0x40E4, 0x39, 0x3, 0x4, 0x232, 0x8009},
	{72000, PIXEL_REPETITION_2, COLOR_DEPTH_16, HDMI_20, 0x7AA0, 0x003D, 0x3, 0x4, 0x14A, 0x8039},
	{73250, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x72, 0x001B, 0x1, 0x4, 0x232, 0x8009},
	{74250, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x72, 0x001B, 0x1, 0x4, 0x232, 0x8009},
	{74250, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, HDMI_14, 0x2145, 0x003F, 0x2, 0x4, 0x232, 0x8009},
	{74250, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, HDMI_14, 0x4061, 0x002E, 0x2, 0x4, 0x232, 0x8009},
	{74250, PIXEL_REPETITION_OFF, COLOR_DEPTH_16, HDMI_14, 0x6071, 0x003F, 0x2, 0x4, 0x230, 0x8009},
	{75000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x72, 0x001B, 0x1, 0x4, 0x232, 0x8009},
	{78750, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x72, 0x001B, 0x1, 0x4, 0x232, 0x8009},
	{79500, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x72, 0x001B, 0x1, 0x4, 0x232, 0x8009},
	{82500, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x72, 0x001B, 0x1, 0x4, 0x232, 0x8009},
	{82500, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, HDMI_14, 0x2145, 0x003F, 0x2, 0x4, 0x232, 0x8009},
	{82500, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, HDMI_14, 0x4061, 0x002E, 0x2, 0x4, 0x232, 0x8009},
	{82500, PIXEL_REPETITION_OFF, COLOR_DEPTH_16, HDMI_14, 0x6071, 0x003F, 0x2, 0x4, 0x230, 0x8009},
	{83500, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x72, 0x001B, 0x1, 0x4, 0x232, 0x8009},
	{85500, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x72, 0x001B, 0x1, 0x4, 0x232, 0x8009},
	{88750, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x72, 0x001B, 0x1, 0x4, 0x232, 0x8009},
	{90000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x72, 0x001B, 0x1, 0x4, 0x232, 0x8009},
	{90000, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, HDMI_14, 0x2145, 0x003F, 0x2, 0x4, 0x232, 0x8009},
	{90000, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, HDMI_14, 0x4061, 0x002E, 0x2, 0x4, 0x232, 0x8009},
	{90000, PIXEL_REPETITION_OFF, COLOR_DEPTH_16, HDMI_14, 0x6071, 0x003F, 0x2, 0x4, 0x230, 0x8009},
	{94500, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x51, 0x001C, 0x2, 0x4, 0x232, 0x8009},
	{99000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x51, 0x001C, 0x2, 0x4, 0x232, 0x8009},
	{99000, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, HDMI_14, 0x2145, 0x003F, 0x2, 0x4, 0x232, 0x8009},
	{99000, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, HDMI_14, 0x4061, 0x002E, 0x2, 0x4, 0x230, 0x8009},
	{99000, PIXEL_REPETITION_OFF, COLOR_DEPTH_16, HDMI_14, 0x6050, 0x003F, 0x3, 0x4, 0x230, 0x8009},
	{100700, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x51, 0x001C, 0x2, 0x4, 0x232, 0x8009},
	{100700, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, HDMI_14, 0x2145, 0x003F, 0x2, 0x4, 0x232, 0x8009},
	{100700, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, HDMI_14, 0x4061, 0x002E, 0x2, 0x4, 0x230, 0x8009},
	{100700, PIXEL_REPETITION_OFF, COLOR_DEPTH_16, HDMI_14, 0x6050, 0x003F, 0x3, 0x4, 0x230, 0x8009},
	{101000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x51, 0x001C, 0x2, 0x4, 0x232, 0x8009},
	{102250, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x51, 0x001C, 0x2, 0x4, 0x232, 0x8009},
	{106500, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x51, 0x001C, 0x2, 0x4, 0x232, 0x8009},
	{108000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x51, 0x001C, 0x2, 0x4, 0x232, 0x8009},
	{108000, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, HDMI_14, 0x2145, 0x003F, 0x2, 0x4, 0x232, 0x8009},
	{108000, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, HDMI_14, 0x4061, 0x002E, 0x2, 0x4, 0x230, 0x8009},
	{108000, PIXEL_REPETITION_OFF, COLOR_DEPTH_16, HDMI_14, 0x6050, 0x003F, 0x3, 0x4, 0x230, 0x8009},
	{108000, PIXEL_REPETITION_1, COLOR_DEPTH_8, HDMI_14, 0x50, 0x003F, 0x3, 0x4, 0x232, 0x8009},
	{108000, PIXEL_REPETITION_1, COLOR_DEPTH_10, HDMI_14, 0x2144, 0x003D, 0x3, 0x4, 0x232, 0x8009},
	{108000, PIXEL_REPETITION_1, COLOR_DEPTH_12, HDMI_14, 0x4060, 0x003F, 0x3, 0x4, 0x230, 0x8009},
	{108000, PIXEL_REPETITION_1, COLOR_DEPTH_16, HDMI_20, 0x7A70, 0x003E, 0x3, 0x4, 0x14A, 0x8039},
	{108000, PIXEL_REPETITION_2, COLOR_DEPTH_8, HDMI_20, 0x1A70, 0x003E, 0x3, 0x4, 0x14A, 0x8039},
	{108000, PIXEL_REPETITION_2, COLOR_DEPTH_10, HDMI_20, 0x3B40, 0x003D, 0x3, 0x4, 0x14A, 0x8039},
	{115500, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x51, 0x001C, 0x2, 0x4, 0x232, 0x8009},
	{117500, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x51, 0x001C, 0x2, 0x4, 0x232, 0x8009},
	{118800, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x51, 0x001C, 0x2, 0x4, 0x232, 0x8009},
	{118800, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, HDMI_14, 0x2145, 0x003F, 0x2, 0x4, 0x230, 0x8009},
	{118800, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, HDMI_14, 0x4061, 0x002E, 0x2, 0x4, 0x230, 0x8009},
	{118800, PIXEL_REPETITION_OFF, COLOR_DEPTH_16, HDMI_14, 0x6050, 0x003F, 0x3, 0x4, 0x273, 0x8009},
	{119000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x51, 0x001C, 0x2, 0x4, 0x232, 0x8009},
	{121750, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x51, 0x001C, 0x2, 0x4, 0x232, 0x8009},
	{122500, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x51, 0x001C, 0x2, 0x4, 0x232, 0x8009},
	{135000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x51, 0x001C, 0x2, 0x4, 0x232, 0x8009},
	{136750, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x51, 0x001C, 0x2, 0x4, 0x232, 0x8009},
	{140250, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x51, 0x001C, 0x2, 0x4, 0x232, 0x8009},
	{144000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x51, 0x001C, 0x2, 0x4, 0x232, 0x8009},
	{144000, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, HDMI_14, 0x2145, 0x003F, 0x2, 0x4, 0x230, 0x8009},
	{144000, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, HDMI_14, 0x4064, 0x003F, 0x3, 0x4, 0x230, 0x8009},
	{144000, PIXEL_REPETITION_OFF, COLOR_DEPTH_16, HDMI_14, 0x6050, 0x003F, 0x3, 0x4, 0x273, 0x8009},
	{146250, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x51, 0x001C, 0x2, 0x4, 0x232, 0x8009},
	{148250, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x51, 0x001C, 0x2, 0x4, 0x232, 0x8009},
	{148500, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x51, 0x001C, 0x2, 0x4, 0x230, 0x8009},
	{148500, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, HDMI_14, 0x214C, 0x003D, 0x3, 0x4, 0x230, 0x8009},
	{148500, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, HDMI_14, 0x4064, 0x003F, 0x3, 0x4, 0x273, 0x8009},
	{148500, PIXEL_REPETITION_OFF, COLOR_DEPTH_16, HDMI_14, 0x6050, 0x003F, 0x3, 0x4, 0x273, 0x8029},
	{154000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x51, 0x001C, 0x2, 0x4, 0x230, 0x8009},
	{156000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x51, 0x001C, 0x2, 0x4, 0x230, 0x8009},
	{157000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x51, 0x001C, 0x2, 0x4, 0x230, 0x8009},
	{157500, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x51, 0x001C, 0x2, 0x4, 0x230, 0x8009},
	{162000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x51, 0x001C, 0x2, 0x4, 0x230, 0x8009},
	{165000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x51, 0x001C, 0x2, 0x4, 0x230, 0x8009},
	{165000, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, HDMI_14, 0x214C, 0x003D, 0x3, 0x4, 0x230, 0x8009},
	{165000, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, HDMI_14, 0x4064, 0x003F, 0x3, 0x4, 0x273, 0x8009},
	{165000, PIXEL_REPETITION_OFF, COLOR_DEPTH_16, HDMI_14, 0x6050, 0x003F, 0x3, 0x4, 0x273, 0x8029},
	{175500, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x51, 0x001C, 0x2, 0x4, 0x230, 0x8009},
	{179500, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x51, 0x001C, 0x2, 0x4, 0x230, 0x8009},
	{180000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x51, 0x001C, 0x2, 0x4, 0x230, 0x8009},
	{180000, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, HDMI_14, 0x214C, 0x003D, 0x3, 0x4, 0x273, 0x8009},
	{180000, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, HDMI_14, 0x4064, 0x003F, 0x3, 0x4, 0x273, 0x8009},
	{180000, PIXEL_REPETITION_OFF, COLOR_DEPTH_16, HDMI_20, 0x7A50, 0x30, 0x3, 0x4, 0x14A, 0x8039},
	{182750, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x51, 0x001C, 0x2, 0x4, 0x230, 0x8009},
	{185625, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x40, 0x001D, 0x3, 0x4, 0x230, 0x8009},
	{185625, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, HDMI_14, 0x214C, 0x003D, 0x3, 0x4, 0x273, 0x8009},
	{185625, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, HDMI_14, 0x4064, 0x003F, 0x3, 0x4, 0x273, 0x8009},
	{185625, PIXEL_REPETITION_OFF, COLOR_DEPTH_16, HDMI_20, 0x7A50, 0x30, 0x3, 0x4, 0x14A, 0x8039},
	{187000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x40, 0x001D, 0x3, 0x4, 0x230, 0x8009},
	{187250, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x40, 0x001D, 0x3, 0x4, 0x230, 0x8009},
	{189000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x40, 0x001D, 0x3, 0x4, 0x230, 0x8009},
	{193250, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x40, 0x001D, 0x3, 0x4, 0x230, 0x8009},
	{198000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x40, 0x001D, 0x3, 0x4, 0x230, 0x8009},
	{198000, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, HDMI_14, 0x214C, 0x003D, 0x3, 0x4, 0x273, 0x8009},
	{198000, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, HDMI_14, 0x4064, 0x003F, 0x3, 0x4, 0x273, 0x8029},
	{198000, PIXEL_REPETITION_OFF, COLOR_DEPTH_16, HDMI_20, 0x7A50, 0x30, 0x3, 0x4, 0x14A, 0x8039},
	{202500, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x40, 0x001D, 0x3, 0x4, 0x230, 0x8009},
	{204750, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x40, 0x001D, 0x3, 0x4, 0x230, 0x8009},
	{208000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x40, 0x001D, 0x3, 0x4, 0x230, 0x8009},
	{214750, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x40, 0x001D, 0x3, 0x4, 0x230, 0x8009},
	{216000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x40, 0x001D, 0x3, 0x4, 0x230, 0x8009},
	{216000, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, HDMI_14, 0x214C, 0x003D, 0x3, 0x4, 0x273, 0x8009},
	{216000, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, HDMI_14, 0x4064, 0x003F, 0x3, 0x4, 0x273, 0x8029},
	{216000, PIXEL_REPETITION_OFF, COLOR_DEPTH_16, HDMI_20, 0x7A50, 0x30, 0x3, 0x4, 0x14A, 0x8039},
	{216000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_20, 0x1A50, 0x30, 0x3, 0x4, 0x14A, 0x8039},
	{216000, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, HDMI_20, 0x3B44, 0x003D, 0x3, 0x4, 0x14A, 0x8039},
	{218250, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x40, 0x001D, 0x3, 0x4, 0x230, 0x8009},
	{229500, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x40, 0x001D, 0x3, 0x4, 0x273, 0x8009},
	{234000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x40, 0x001D, 0x3, 0x4, 0x273, 0x8009},
	{237600, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x40, 0x001D, 0x3, 0x4, 0x273, 0x8009},
	{237600, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, HDMI_14, 0x214C, 0x003D, 0x3, 0x4, 0x273, 0x8029},
	{237600, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, HDMI_20, 0x5A64, 0x002A, 0x3, 0x4, 0x14A, 0x8039},
	{237600, PIXEL_REPETITION_OFF, COLOR_DEPTH_16, HDMI_20, 0x7A50, 0x30, 0x3, 0x4, 0x14A, 0x8039},
	{245250, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x40, 0x001D, 0x3, 0x4, 0x273, 0x8009},
	{245500, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x40, 0x001D, 0x3, 0x4, 0x273, 0x8009},
	{261000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x40, 0x001D, 0x3, 0x4, 0x273, 0x8009},
	{268250, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x40, 0x001D, 0x3, 0x4, 0x273, 0x8009},
	{268500, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x40, 0x001D, 0x3, 0x4, 0x273, 0x8009},
	{281250, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x40, 0x001D, 0x3, 0x4, 0x273, 0x8009},
	{288000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x40, 0x001D, 0x3, 0x4, 0x273, 0x8009},
	{288000, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, HDMI_20, 0x3B4C, 0x003D, 0x3, 0x4, 0x14A, 0x8039},
	{288000, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, HDMI_20, 0x5A64, 0x002A, 0x3, 0x4, 0x14A, 0x8039},
	{288000, PIXEL_REPETITION_OFF, COLOR_DEPTH_16, HDMI_20, 0x7A50, 0x22, 0x3, 0x4, 0x14A, 0x8039},
	{297000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x40, 0x001D, 0x3, 0x4, 0x273, 0x8029},
	{297000, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, HDMI_20, 0x3B4C, 0x003D, 0x3, 0x4, 0x14A, 0x8039},
	{297000, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, HDMI_20, 0x5A64, 0x002A, 0x3, 0x4, 0x14A, 0x8039},
	{297000, PIXEL_REPETITION_OFF, COLOR_DEPTH_16, HDMI_20, 0x7A50, 0x22, 0x3, 0x4, 0x14A, 0x8039},
	{317000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x40, 0x001D, 0x3, 0x4, 0x273, 0x8029},
	{330000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x40, 0x001D, 0x3, 0x4, 0x273, 0x8029},
	{330000, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, HDMI_20, 0x3B4C, 0x003D, 0x3, 0x4, 0x14A, 0x8039},
	{330000, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, HDMI_20, 0x5A64, 0x002A, 0x3, 0x4, 0x14A, 0x8039},
	{333250, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x40, 0x001D, 0x3, 0x4, 0x273, 0x8029},
	{340000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_14, 0x40, 0x001D, 0x3, 0x4, 0x273, 0x8029},
	{348500, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_20, 0x1A40, 0x8, 0x3, 0x4, 0x14A, 0x8039},
	{356500, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_20, 0x1A40, 0x8, 0x3, 0x4, 0x14A, 0x8039},
	{360000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_20, 0x1A40, 0x8, 0x3, 0x4, 0x14A, 0x8039},
	{360000, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, HDMI_20, 0x3B4C, 0x003D, 0x3, 0x4, 0x14A, 0x8039},
	{360000, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, HDMI_20, 0x5A64, 0x002A, 0x3, 0x4, 0x14A, 0x8039},
	{371250, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_20, 0x1A40, 0x8, 0x3, 0x4, 0x14A, 0x8039},
	{371250, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, HDMI_20, 0x3B4C, 0x003D, 0x3, 0x4, 0x14A, 0x8039},
	{371250, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, HDMI_20, 0x5A64, 0x002A, 0x3, 0x4, 0x14A, 0x8039},
	{380500, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_20, 0x1A40, 0x8, 0x3, 0x4, 0x14A, 0x8039},
	{396000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_20, 0x1A40, 0x8, 0x3, 0x4, 0x14A, 0x8039},
	{396000, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, HDMI_20, 0x3B4C, 0x003D, 0x3, 0x4, 0x14A, 0x8039},
	{396000, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, HDMI_20, 0x5A64, 0x002A, 0x3, 0x4, 0x14A, 0x8039},
	{432000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_20, 0x1A40, 0x8, 0x3, 0x4, 0x14A, 0x8039},
	{432000, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, HDMI_20, 0x3B4C, 0x003D, 0x3, 0x4, 0x14A, 0x8039},
	{443250, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_20, 0x1A40, 0x8, 0x3, 0x4, 0x14A, 0x8039},
	{475200, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_20, 0x1A40, 0x8, 0x3, 0x4, 0x14A, 0x8039},
	{475200, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, HDMI_20, 0x3B4C, 0x003D, 0x3, 0x4, 0x14A, 0x8039},
	{495000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_20, 0x1A40, 0x8, 0x3, 0x4, 0x14A, 0x8039},
	{505250, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_20, 0x1A40, 0x8, 0x3, 0x4, 0x14A, 0x8039},
	{552750, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_20, 0x1A40, 0x8, 0x3, 0x4, 0x14A, 0x8039},
	{594000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, HDMI_20, 0x1A40, 0x8, 0x3, 0x4, 0x14A, 0x8039},
	{0,  PIXEL_REPETITION_OFF, COLOR_DEPTH_INVALID, HDMI_14, 0, 0, 0, 0, 0, 0}
};
#endif

static u32 phy301_get_freq(u32 pClk)
{
	if (((pClk >= 25175) && (pClk <= 25180)) ||
			((pClk >= 25195) && (pClk <= 25205)))
		return 25175;
	else if (((pClk >= 26995) && (pClk <= 27005)) ||
			((pClk >= 27022) && (pClk <= 27032)))
		return 27000;
	else if (pClk == 31500)
		return 31500;
	else if (pClk == 33750)
		return 33750;
	else if (pClk == 35500)
		return 35500;
	else if (((pClk >= 35995) && (pClk <= 36005)) ||
			((pClk >= 36031) && (pClk <= 36041)))
		return 36000;
	else if (pClk == 40000)
		return 40000;
	else if (pClk == 44900)
		return 44900;
	else if (pClk == 49500)
		return 49500;
	else if (pClk == 50000)
		return 50000;
	else if (((pClk >= 50345) && (pClk <= 50355)) ||
					((pClk >= 50395) && (pClk <= 50405)))
		return 50350;
	else if (((pClk >= 53995) && (pClk <= 54005)) ||
					((pClk >= 50049) && (pClk <= 54059)))
		return 54000;
	else if (pClk == 56250)
		return 56250;
	else if (((pClk >= 59336) && (pClk <= 59346)) ||
					((pClk >= 59395) && (pClk <= 59405)))
		return 59400;
	else if (pClk == 65000)
		return 65000;
	else if (pClk == 68250)
		return 68250;
	else if (pClk == 71000)
		return 71000;
	else if (pClk == 72000)
		return 72000;
	else if (pClk == 73250)
		return 73250;
	else if (((pClk >= 74171) && (pClk <= 74181)) ||
					((pClk >= 74245) && (pClk <= 74255)))
		return 74250;
	else if (pClk == 75000)
		return 75000;
	else if (pClk == 78750)
		return 78750;
	else if (pClk == 79500)
		return 79500;
	else if (((pClk >= 82143) && (pClk <= 82153)) ||
					((pClk >= 82495) && (pClk <= 82505)))
		return 82500;
	else if (pClk == 83500)
		return 83500;
	else if (pClk == 85500)
		return 85500;
	else if (pClk == 88750)
		return 88750;
	else if (pClk == 90000)
		return 90000;
	else if (pClk == 94500)
		return 94500;
	else if (((pClk >= 98896) && (pClk <= 98906)) ||
					((pClk >= 98995) && (pClk <= 99005)))
		return 99000;
	else if (((pClk >= 100695) && (pClk <= 100705)) ||
					((pClk >= 100795) && (pClk <= 100805)))
		return 100700;
	else if (pClk == 101000)
		return  101000;
	else if (pClk == 102250)
		return 102250;
	else if (pClk == 106500)
		return 106500;
	else if (((pClk > 107994) && (pClk < 108006)) ||
					((pClk > 108102) && (pClk < 108114)))
		return 108000;
	else if (pClk == 115500)
		return 115500;
	else if (pClk == 117500)
		return 117500;
	else if (((pClk >= 118795) && (pClk <= 118805)) ||
					((pClk >= 118677) && (pClk <= 118687)))
		return 118800;
	else if (pClk == 119000)
		return 119000;
	else if (pClk == 121750)
		return 122500;
	else if (pClk == 122500)
		return 121750;
	else if (pClk == 135000)
		return 135000;
	else if (pClk == 136750)
		return 136750;
	else if (pClk == 140250)
		return 140250;
	else if ((pClk == 144000) || (pClk == 142250) || (pClk == 142000))
		return 144000;
	else if (pClk == 146250)
		return 146250;
	else if (pClk == 148250)
		return 148250;
	else if (((pClk >= 148347) && (pClk <= 148357)) ||
					((pClk >= 148495) && (pClk <= 148505)))
		return 148500;
	else if (pClk == 154000)
		return 154000;
	else if (pClk == 156000)
		return 156000;
	else if (pClk == 157000)
		return 157000;
	else if (pClk == 157500)
		return 157500;
	else if (pClk == 162000)
		return 162000;
	else if (((pClk >= 164830) && (pClk <= 164840)) ||
					((pClk >= 164995) && (pClk <= 165005)))
		return 165000;
	else if (pClk == 175500)
		return 175500;
	else if (pClk == 179500)
		return 179500;
	else if (pClk == 180000)
		return 180000;
	else if (pClk == 182750)
		return 182750;
	else if (((pClk >= 185435) && (pClk <= 185445)) ||
				((pClk >= 185620) && (pClk <= 185630)))
		return 185625;
	else if (pClk == 187000)
		return 187000;
	else if (pClk == 187250)
		return 187250;
	else if (pClk == 189000)
		return 189000;
	else if (pClk == 193250)
		return 193250;
	else if (((pClk >= 197797) && (pClk <= 197807)) ||
				((pClk >= 197995) && (pClk <= 198005)))
		return 198000;
	else if (pClk == 202500)
		return 202500;
	else if (pClk == 204750)
		return 204750;
	else if (pClk == 208000)
		return 208000;
	else if (pClk == 214750)
		return 214750;
	else if (((pClk >= 216211) && (pClk <= 216221)) ||
				((pClk >= 215995) && (pClk <= 216005)))
		return 216000;
	else if (pClk == 218250)
		return 218250;
	else if (pClk == 229500)
		return 229500;
	else if ((pClk == 234000) || (pClk == 233500))
		return 234000;
	else if (((pClk >= 237359) && (pClk <= 237369)) ||
				((pClk >= 237595) && (pClk <= 237605)))
		return 237600;
	else if (pClk == 245250)
		return 245250;
	else if (pClk == 245500)
		return 245500;
	else if (pClk == 261000)
		return 261000;
	else if (pClk == 268250)
		return 268250;
	else if (pClk == 268500)
		return 268500;
	else if ((pClk == 281250) || (pClk == 280000))
		return 281250;
	else if (pClk == 288000)
		return 288000;
	else if (((pClk >= 296698) && (pClk <= 296708)) ||
				((pClk >= 296995) && (pClk <= 297005)))
		return 297000;
	else if (pClk == 317000)
		return 317000;
	else if (pClk == 330000)
		return 330000;
	else if (pClk == 333250)
		return 333250;
	else if (((pClk >= 339655) && (pClk <= 339665)) ||
				((pClk >= 339995) && (pClk <= 340005)))
		return 340000;
	else if (pClk == 348500)
		return 348500;
	else if (pClk == 356500)
		return 356500;
	else if (pClk == 360000)
		return 360000;
	else if (((pClk >= 370874) && (pClk <= 370884)) ||
				((pClk >= 371245) && (pClk <= 371255)))
		return 371250;
	else if (pClk == 380500)
		return 380500;
	else if (((pClk >= 395599) && (pClk <= 395609)) ||
				((pClk >= 395995) && (pClk <= 396005)))
		return 396000;
	else if (((pClk >= 431952) && (pClk <= 431967)) ||
			((pClk >= 431995) && (pClk <= 432005)) ||
			((pClk >= 432427) && (pClk <= 432437)))
		return 432000;
	else if (pClk == 443250)
		return 443250;
	else if (((pClk >= 475148) && (pClk <= 475158)) ||
		((pClk >= 475195) && (pClk <= 475205)) ||
		((pClk >= 474723) && (pClk <= 474733)))
		return 475200;
	else if (((pClk >= 494500) && (pClk <= 494510)) ||
		((pClk >= 494995) && (pClk <= 495005)))
		return 495000;
	else if (pClk == 505250)
		return 505250;
	else if (pClk == 552750)
		return 552750;
	else if (((pClk >= 593995) && (pClk <= 594005)) ||
		((pClk >= 593403) && (pClk <= 593413)))
		return 594000;
	else {
		pr_err("Error:Unable to map input pixel clock frequency %d kHz\n", pClk);
	}
	return 1000;
}

#ifdef SUPPORT_PHY303
u32 phy303_get_freq(u32 pClk)
{
	if (((pClk >= 25175) && (pClk <= 25180)) ||
			((pClk >= 25195) && (pClk <= 25205)))
		return 25175;
	else if (((pClk >= 26995) && (pClk <= 27005)) ||
				((pClk >= 27022) && (pClk <= 27032)))
		return 27000;
	else if (pClk == 31500)
		return 31500;
	else if (pClk == 33750)
		return 33750;
	else if (pClk == 35500)
		return 35500;
	else if (((pClk >= 35995) && (pClk <= 36005)) ||
			((pClk >= 36031) && (pClk <= 36041)))
		return 36000;
	else if (pClk == 40000)
		return 40000;
	else if (pClk == 44900)
		return 44900;
	else if (pClk == 49500)
		return 49500;
	else if (pClk == 50000)
		return 50000;
	else if (((pClk >= 50345) && (pClk <= 50355)) ||
				((pClk >= 50395) && (pClk <= 50405)))
		return 50350;
	else if (((pClk >= 53995) && (pClk <= 54005)) ||
				((pClk >= 50049) && (pClk <= 54059)))
		return 54000;
	else if (pClk == 56250)
		return 56250;
	else if (((pClk >= 59336) && (pClk <= 59346)) ||
				((pClk >= 59395) && (pClk <= 59405)))
		return 59400;
	else if (pClk == 65000)
		return 65000;
	else if (pClk == 68250)
		return 68250;
	else if (pClk == 71000)
		return 71000;
	else if (pClk == 72000)
		return 72000;
	else if (pClk == 73250)
		return 73250;
	else if (((pClk >= 74171) && (pClk <= 74181)) ||
				((pClk >= 74245) && (pClk <= 74255)))
		return 74250;
	else if (pClk == 75000)
		return 75000;
	else if (pClk == 78750)
		return 78750;
	else if (pClk == 79500)
		return 79500;
	else if (((pClk >= 82143) && (pClk <= 82153)) ||
				((pClk >= 82495) && (pClk <= 82505)))
		return 82500;
	else if (pClk == 83500)
		return 83500;
	else if (pClk == 85500)
		return 85500;
	else if (pClk == 88750)
		return 88750;
	else if (pClk == 90000)
		return 90000;
	else if (pClk == 94500)
		return 94500;
	else if (((pClk >= 98896) && (pClk <= 98906)) ||
				((pClk >= 98995) && (pClk <= 99005)))
		return 99000;
	else if (((pClk >= 100695) && (pClk <= 100705)) ||
				((pClk >= 100795) && (pClk <= 100805)))
		return 100700;
	else if (pClk == 101000)
		return 101000;
	else if (pClk == 102250)
		return 102250;
	else if (pClk == 106500)
		return 106500;
	else if (((pClk > 107994) && (pClk < 108006)) ||
					((pClk > 108102) && (pClk < 108114)))
		return 108000;
	else if (pClk == 115500)
		return 115500;
	else if (pClk == 117500)
		return 117500;
	else if (((pClk >= 118795) && (pClk <= 118805)) ||
			((pClk >= 118677) && (pClk <= 118687)))
		return 118800;
	else if (pClk == 119000)
		return 119000;
	else if (pClk == 121750)
		return 122500;
	else if (pClk == 122500)
		return 121750;
	else if (pClk == 135000)
		return 135000;
	else if (pClk == 136750)
		return 136750;
	else if (pClk == 140250)
		return 140250;
	else if (pClk == 144000)
		return 144000;
	else if (pClk == 146250)
		return 146250;
	else if (pClk == 148250)
		return 148250;
	else if (((pClk >= 148347) && (pClk <= 148357)) ||
				((pClk >= 148495) && (pClk <= 148505)))
		return 148500;
	else if (pClk == 154000)
		return 154000;
	else if (pClk == 156000)
		return 156000;
	else if (pClk == 157000)
		return 157000;
	else if (pClk == 157500)
		return 157500;
	else if (pClk == 162000)
		return 162000;
	else if (((pClk >= 164830) && (pClk <= 164840)) ||
				((pClk >= 164995) && (pClk <= 165005)))
		return 165000;
	else if (pClk == 175500)
		return 175500;
	else if (pClk == 179500)
		return 179500;
	else if (pClk == 180000)
		return 180000;
	else if (pClk == 182750)
		return 182750;
	else if (((pClk >= 185435) && (pClk <= 185445)) ||
				((pClk >= 185620) && (pClk <= 185630)))
		return 185625;
	else if (pClk == 187000)
		return 187000;
	else if (pClk == 187250)
		return 187250;
	else if (pClk == 189000)
		return 189000;
	else if (pClk == 193250)
		return 193250;
	else if (((pClk >= 197797) && (pClk <= 197807)) ||
				((pClk >= 197995) && (pClk <= 198005)))
		return 198000;
	else if (pClk == 202500)
		return 202500;
	else if (pClk == 204750)
		return 204750;
	else if (pClk == 208000)
		return 208000;
	else if (pClk == 214750)
		return 214750;
	else if (((pClk >= 216211) && (pClk <= 216221)) ||
				((pClk >= 215995) && (pClk <= 216005)))
		return 216000;
	else if (pClk == 218250)
		return 218250;
	else if (pClk == 229500)
		return 229500;
	else if (pClk == 234000)
		return 234000;
	else if (((pClk >= 237359) && (pClk <= 237369)) ||
				((pClk >= 237595) && (pClk <= 237605)))
		return 237600;
	else if (pClk == 245250)
		return 245250;
	else if (pClk == 245500)
		return 245500;
	else if (pClk == 261000)
		return 261000;
	else if (pClk == 268250)
		return 268250;
	else if (pClk == 268500)
		return 268500;
	else if (pClk == 281250)
		return 281250;
	else if (pClk == 288000)
		return 288000;
	else if (((pClk >= 296698) && (pClk <= 296708)) ||
				((pClk >= 296995) && (pClk <= 297005)))
		return 297000;
	else if (pClk == 317000)
		return 317000;
	else if (pClk == 330000)
		return 330000;
	else if (pClk == 333250)
		return 333250;
	else if (((pClk >= 339655) && (pClk <= 339665)) ||
			((pClk >= 339995) && (pClk <= 340005)))
		return 340000;
	else if (pClk == 348500)
		return 348500;
	else if (pClk == 356500)
		return 356500;
	else if (pClk == 360000)
		return 360000;
	else if (((pClk >= 370874) && (pClk <= 370884)) ||
				((pClk >= 371245) && (pClk <= 371255)))
		return 371250;
	else if (pClk == 380500)
		return 380500;
	else if (((pClk >= 395599) && (pClk <= 395609)) ||
				((pClk >= 395995) && (pClk <= 396005)))
		return 396000;
	else if (((pClk >= 431952) && (pClk <= 431967)) ||
			((pClk >= 431995) && (pClk <= 432005)) ||
			((pClk >= 432427) && (pClk <= 432437)))
		return 432000;
	else if (pClk == 443250)
		return 443250;
	else if (((pClk >= 475148) && (pClk <= 475158)) ||
			((pClk >= 475195) && (pClk <= 475205)) ||
			((pClk >= 474723) && (pClk <= 474733)))
		return 475200;
	else if (((pClk >= 494500) && (pClk <= 494510)) ||
			((pClk >= 494995) && (pClk <= 495005)))
		return 495000;
	else if (pClk == 505250)
		return 505250;
	else if (pClk == 552750)
		return 552750;
	else if (((pClk >= 593995) && (pClk <= 594005)) ||
			((pClk >= 593403) && (pClk <= 593413)))
		return 594000;
	else {
		pr_err("Error:Unable to map input pixel clock frequency %dkHz\n", pClk);
	}
	return 1000;
}
#endif

static struct phy_config *phy301_get_configs(u32 pClk, color_depth_t color,
		pixel_repetition_t pixel)
{

	int i = 0;

	for (i = 0; phy301[i].clock != 0; i++) {

		pClk = phy301_get_freq(pClk);

		if ((pClk == phy301[i].clock) &&
				(color == phy301[i].color) &&
				(pixel == phy301[i].pixel)) {
			return &(phy301[i]);
		}
	}
	pr_info("EEROR: do not get phy_config\n");

	return NULL;
}

#ifdef SUPPORT_PHY303
static struct phy_config303 *phy303_get_configs(u32 pClk, color_depth_t color,
		pixel_repetition_t pixel)
{

	int i = 0;

	for (i = 0; phy303[i].clock != 0; i++) {

		pClk = phy303_get_freq(pClk);

		if ((pClk == phy303[i].clock) &&
				(color == phy303[i].color) &&
				(pixel == phy303[i].pixel)) {
			return &(phy303[i]);
		}
	}

	return NULL;
}
#endif

static int phy301_configure(hdmi_tx_dev_t *dev, u32 pClk, color_depth_t color,
						pixel_repetition_t pixel)
{
	int i   = 0;
	u16 phyRead = 0;
	u8 lock = 0;
	struct phy_config *config = NULL;
	/* value = 0; */

	LOG_TRACE();
	/* Color resolution 0 is 8 bit color depth */
	if (color == 0)
		color = COLOR_DEPTH_8;

	config = phy301_get_configs(pClk, color, pixel);

	if (config == NULL) {
		pr_err("Error:failed to get phy config\n");
		pr_err("Configuration for clk %dkhz color depth %d pixel repetition %d\n",
				  pClk, color, pixel);
		return -1;
	}

	mc_phy_reset(dev, 1);

	dev_write_mask(dev, PHY_CONF0, PHY_CONF0_TXPWRON_MASK, 0);
	dev_write_mask(dev, PHY_CONF0, PHY_CONF0_PDDQ_MASK, 1);
	dev_write_mask(dev, PHY_CONF0, PHY_CONF0_SVSRET_MASK, 0);

	dev_write_mask(dev, PHY_CONF0, PHY_CONF0_SVSRET_MASK, 1);

	mc_phy_reset(dev, 0);

	/* dev->snps_hdmi_ctrl.phy_access = PHY_I2C; */
	phy_reconfigure_interface(dev);

	phy_write(dev, OPMODE_PLLCFG, config->oppllcfg);
	if (phy_read(dev, OPMODE_PLLCFG, &phyRead) || (phyRead != config->oppllcfg))
		pr_err("Error:OPMODE_PLLCFG Mismatch Write 0x%04x Read 0x%04x\n",
				config->oppllcfg, phyRead);

	phy_write(dev, PLLCURRCTRL, config->pllcurrctrl);
	if (phy_read(dev, PLLCURRCTRL, &phyRead) || (phyRead != config->pllcurrctrl))
		pr_err("Error:PLLCURRCTRL Mismatch Write 0x%04x Read 0x%04x\n",
				config->pllcurrctrl, phyRead);

#if IS_ENABLED(CONFIG_ARCH_SUN50IW9)
	phy_write(dev, PLLGMPCTRL, 3);
	phy_read(dev, PLLGMPCTRL, &phyRead);
	if (phyRead != 3)
		pr_err("Error:PLLGMPCTRL Mismatch Write 0x%04x Read 0x%04x\n",
				config->pllgmpctrl, phyRead);
#else
	phy_write(dev, PLLGMPCTRL, config->pllgmpctrl);
	phy_read(dev, PLLGMPCTRL, &phyRead);
	if (phyRead != config->pllgmpctrl)
		pr_err("Error:PLLGMPCTRL Mismatch Write 0x%04x Read 0x%04x\n",
				config->pllgmpctrl, phyRead);
#endif

	phy_write(dev, TXTERM, config->txterm);
	if (phy_read(dev, TXTERM, &phyRead) || (phyRead != config->txterm))
			pr_err("Error:TXTERM Mismatch Write 0x%04x Read 0x%04x\n",
					config->txterm, phyRead);


	phy_write(dev, CKSYMTXCTRL, config->cksymtxctrl);
	if (phy_read(dev, CKSYMTXCTRL, &phyRead) || (phyRead != config->cksymtxctrl))
		pr_err("Error:CKSYMTXCTRL Mismatch Write 0x%04x Read 0x%04x\n",
				config->cksymtxctrl, phyRead);

	phy_write(dev, VLEVCTRL, config->vlevctrl);
	if (phy_read(dev, VLEVCTRL, &phyRead) || (phyRead != config->vlevctrl))
		pr_err("Error:VLEVCTRL Mismatch Write 0x%04x Read 0x%04x\n",
				config->vlevctrl, phyRead);

	if (dev->snps_hdmi_ctrl.pixel_clock == 594000) {
		phy_write(dev, 0x17, 0x8006);
		if (phy_read(dev, 0x17, &phyRead) || (phyRead != 0x8006))
			pr_err("Error:VLEVCTRL Mismatch Write 0x8006 Read 0x%04x\n",
							phyRead);
	}

	dev_write_mask(dev, PHY_CONF0, PHY_CONF0_PDDQ_MASK, 0);
	dev_write_mask(dev, PHY_CONF0, PHY_CONF0_TXPWRON_MASK, 1);

	/* wait PHY_TIMEOUT no of cycles at most for
	the PLL lock signal to raise ~around 20us max */
	for (i = 0; i < PHY_TIMEOUT; i++) {
		snps_sleep(5);
		lock = phy_phase_lock_loop_state(dev);
		if (lock & 0x1) {
			VIDEO_INF("PHY PLL locked\n");
			return TRUE;
		}
	}

	pr_err("PHY PLL not locked\n");
	return FALSE;
}

#ifdef SUPPORT_PHY303
static int phy303_configure(hdmi_tx_dev_t *dev, u32 pClk, color_depth_t color,
						pixel_repetition_t pixel)
{
	int i   = 0;
	u16 phyRead = 0;
	u8 lock = 0;
	struct phy_config303 *config = NULL;
	u32 value = 0;

	/* Color resolution 0 is 8 bit color depth */
	if (color == 0)
		color = COLOR_DEPTH_8;

	config = phy303_get_configs(pClk, color, pixel);

	if (config == NULL) {
		pr_err("Error:Configuration for clk %dkhz color depth %d pixel repetition %d\n",
				  pClk, color, pixel);
		return -1;
	}

	value = dev_read(dev, 0xb0020);
	value |= (1 << 9);
	dev_write(dev, 0xb0020, value);

	/* board_ZcalReset(1); */

	mc_phy_reset(dev, 1);

	dev_write_mask(dev, PHY_CONF0, PHY_CONF0_TXPWRON_MASK, 0);
	dev_write_mask(dev, PHY_CONF0, PHY_CONF0_PDDQ_MASK, 1);
	dev_write_mask(dev, PHY_CONF0, PHY_CONF0_SVSRET_MASK, 0);

	/* board_ZcalReset(0); */

	do {
		snps_sleep(5);
	} while (!(dev_read(dev, 0xb0010) & 0x1) && (i++ < PHY_TIMEOUT));

	dev_write_mask(dev, PHY_CONF0, PHY_CONF0_SVSRET_MASK, 1);

	mc_phy_reset(dev, 0);

	phy_reconfigure_interface(dev);

	phy_write(dev, OPMODE_PLLCFG, config->oppllcfg);
	phy_read(dev, OPMODE_PLLCFG, &phyRead);
	if (phyRead != config->oppllcfg)
		pr_err("Error:OPMODE_PLLCFG Mismatch Write 0x%04x Read 0x%04x\n",
				config->oppllcfg, phyRead);

	phy_write(dev, PLLCURRCTRL, config->pllcurrctrl);
	phy_read(dev, PLLCURRCTRL, &phyRead);
	if (phyRead != config->pllcurrctrl)
		pr_err("Error:PLLCURRCTRL Mismatch Write 0x%04x Read 0x%04x\n",
				config->pllcurrctrl, phyRead);

	phy_write(dev, PLLGMPCTRL, config->pllgmpctrl);
	phy_read(dev, PLLGMPCTRL, &phyRead);
	if (phyRead != config->pllgmpctrl)
		pr_err("Error:PLLGMPCTRL Mismatch Write 0x%04x Read 0x%04x\n",
				config->pllgmpctrl, phyRead);

	phy_write(dev, TXTERM, config->txterm);
	phy_read(dev, TXTERM, &phyRead);
	if (phyRead != config->txterm)
		pr_err("Error:TXTERM Mismatch Write 0x%04x Read 0x%04x\n",
				config->txterm, phyRead);

	phy_write(dev, CKSYMTXCTRL, config->cksymtxctrl);
	phy_read(dev, CKSYMTXCTRL, &phyRead);
	if (phyRead != config->cksymtxctrl)
		pr_err("Error:CKSYMTXCTRL Mismatch Write 0x%04x Read 0x%04x\n",
				config->cksymtxctrl, phyRead);

	phy_write(dev, VLEVCTRL, config->vlevctrl);
	phy_read(dev, VLEVCTRL, &phyRead);
	if (phyRead != config->vlevctrl)
		pr_err("Error:VLEVCTRL Mismatch Write 0x%04x Read 0x%04x\n",
				config->vlevctrl, phyRead);

	dev_write_mask(dev, PHY_CONF0, PHY_CONF0_PDDQ_MASK, 0);
	dev_write_mask(dev, PHY_CONF0, PHY_CONF0_TXPWRON_MASK, 1);

	/* wait PHY_TIMEOUT no of cycles at most for the PLL lock
		signal to raise ~around 20us max */
	for (i = 0; i < PHY_TIMEOUT; i++) {
		snps_sleep(5);
		lock = phy_phase_lock_loop_state(dev);
		if (lock & 0x1) {
			VIDEO_INF("PHY PLL locked\n");
			return TRUE;
		}
	}

	pr_err("PHY PLL not locked\n");
	return FALSE;
}
#endif

static int phy_set_interface(hdmi_tx_dev_t *dev, phy_access_t interface)
{
	/* TODO: should be implemented a lock where to prevent
		PHY access during this configuration */
	if (dev->snps_hdmi_ctrl.phy_access == interface) {
		VIDEO_INF("Phy interface already set to %s",
				interface == PHY_I2C ? "I2C" : "JTAG");

		(dev->snps_hdmi_ctrl.phy_access == PHY_I2C) ?
				phy_slave_address(dev, PHY_I2C_SLAVE_ADDR) : 0;

		return 0;
	}

	switch (interface) {
#ifdef SUPPORT_PHY_JTAG
	case PHY_JTAG:
		phy_jtag_init(dev, 0xD4);
		break;
#endif
	case PHY_I2C:
		dev_write(dev, JTAG_PHY_CONFIG, JTAG_PHY_CONFIG_I2C_JTAGZ_MASK);
		phy_slave_address(dev, PHY_I2C_SLAVE_ADDR);
		break;
	default:
		pr_err("PHY interface not defined");
		return -1;
	}
	dev->snps_hdmi_ctrl.phy_access = interface;
	VIDEO_INF("PHY interface set to %s",
			interface == PHY_I2C ? "I2C" : "JTAG");
	return 0;
}

static int phy_reconfigure_interface(hdmi_tx_dev_t *dev)
{
	switch (dev->snps_hdmi_ctrl.phy_access) {
#ifdef SUPPORT_PHY_JTAG
	case PHY_JTAG:
		phy_jtag_init(dev, 0xD4);
		break;
#endif
	case PHY_I2C:
		dev_write(dev, JTAG_PHY_CONFIG, JTAG_PHY_CONFIG_I2C_JTAGZ_MASK);
		phy_slave_address(dev, PHY_I2C_SLAVE_ADDR);
		break;
	default:
		pr_err("PHY interface not defined");
		return -1;
	}
	VIDEO_INF("PHY interface reconfiguration, set to %s",
		dev->snps_hdmi_ctrl.phy_access == PHY_I2C ? "I2C" : "JTAG");
	return 0;

}

static int phy_slave_address(hdmi_tx_dev_t *dev, u8 value)
{
	switch (dev->snps_hdmi_ctrl.phy_access) {
#ifdef SUPPORT_PHY_JTAG
	case PHY_JTAG:
		phy_jtag_slave_address(dev, 0xD4);
		return 0;
#endif
	case PHY_I2C:
		phy_i2c_slave_address(dev, value);
		return 0;
	default:
		pr_err("PHY interface not defined");
	}
	return -1;
}


int phy_initialize(hdmi_tx_dev_t *dev, u16 phy_model)
{
	LOG_TRACE1(dev->snps_hdmi_ctrl.data_enable_polarity);

	if (phy_set_interface(dev, PHY_I2C) < 0)
		return FALSE;

#if 0
	if (phy_set_interface(dev, PHY_JTAG) < 0)
			return FALSE;

#endif

/* if(dev->snps_hdmi_ctrl.phy_access==PHY_ACCESS_UNDEFINED) { */
	/* if ((id_design(dev) > 0x21) && dev_read(dev, CONFIG2_ID)) { */
		/* phy_set_interface(dev, PHY_JTAG ); */
		/* } else */
		/* phy_set_interface(dev, PHY_I2C ); */
	/* } */
/* } */

#ifndef PHY_THIRD_PARTY
	if (phy_model == PHY_MODEL_108) {
		VIDEO_INF("GEN 2 GF 28SLP 1.8V build - E108\n");
		phy_gen2_tx_power_on(dev, 0);
		phy_gen2_pddq(dev, 1);
	}

	if (phy_model == PHY_MODEL_301) {
		VIDEO_INF("HDMI MHL TSMC 28HPM 1.8V build - E301\n");
		phy_gen2_tx_power_on(dev, 0);
		phy_gen2_pddq(dev, 1);
	}

	if (phy_model == PHY_MODEL_303) {
		VIDEO_INF("HDMI MHL TSMC 28HPM 1.8V build - E303\n");
		phy_gen2_tx_power_on(dev, 0);
		phy_gen2_pddq(dev, 1);
	}

	phy_interrupt_mask(dev, PHY_MASK0_TX_PHY_LOCK_MASK |
					PHY_STAT0_HPD_MASK |
					PHY_MASK0_RX_SENSE_0_MASK |
					PHY_MASK0_RX_SENSE_1_MASK |
					PHY_MASK0_RX_SENSE_2_MASK |
					PHY_MASK0_RX_SENSE_3_MASK);
	phy_data_enable_polarity(dev, dev->snps_hdmi_ctrl.data_enable_polarity);
	phy_interface_control(dev, 0);
	phy_enable_tmds(dev, 0);
	phy_power_down(dev, 0);	/* disable PHY */
	phy_i2c_mask_interrupts(dev, 0);
#else
	VIDEO_INF("Third Party PHY build\n");
#endif

	/* Clean IH_I2CMPHY_STAT0 */
	dev_write_mask(dev, IH_I2CMPHY_STAT0, IH_I2CMPHY_STAT0_I2CMPHYERROR_MASK
					| IH_I2CMPHY_STAT0_I2CMPHYDONE_MASK, 0);

	return TRUE;
}

int phy_configure(hdmi_tx_dev_t *dev, u16 phy_model, encoding_t EncodingOut)
{
	LOG_TRACE();
	EncodingOut = 0;
	if (phy_model == PHY_MODEL_301) {
		return phy301_configure(dev, dev->snps_hdmi_ctrl.pixel_clock,
					dev->snps_hdmi_ctrl.color_resolution,
					dev->snps_hdmi_ctrl.pixel_repetition);
	}

#ifdef SUPPORT_PHY303
	if (phy_model == PHY_MODEL_303) {
		return phy303_configure(dev, dev->snps_hdmi_ctrl.pixel_clock,
					dev->snps_hdmi_ctrl.color_resolution,
					dev->snps_hdmi_ctrl.pixel_repetition);
	}
#endif
	pr_err("PHY not supported %d\n", phy_model);

	return FALSE;
}

int phy_standby(hdmi_tx_dev_t *dev)
{
#ifndef PHY_THIRD_PARTY
	phy_interrupt_mask(dev, PHY_MASK0_TX_PHY_LOCK_MASK |
				PHY_MASK0_RX_SENSE_0_MASK |
				PHY_MASK0_RX_SENSE_1_MASK |
				PHY_MASK0_RX_SENSE_2_MASK |
				PHY_MASK0_RX_SENSE_3_MASK);
				/* mask phy interrupts - leave HPD */
	phy_enable_tmds(dev, 0);
	phy_power_down(dev, 0);	/* disable PHY */
	phy_gen2_tx_power_on(dev, 0);
	phy_gen2_pddq(dev, 1);
#endif
	return TRUE;
}

int phy_enable_hpd_sense(hdmi_tx_dev_t *dev)
{
#ifndef PHY_THIRD_PARTY
	phy_gen2_en_hpd_rx_sense(dev, 1);
#endif
	return TRUE;
}

int phy_disable_hpd_sense(hdmi_tx_dev_t *dev)
{
#ifndef PHY_THIRD_PARTY
	phy_gen2_en_hpd_rx_sense(dev, 0);
#endif
	return TRUE;
}



static int phy_phase_lock_loop_state(hdmi_tx_dev_t *dev)
{
	LOG_TRACE();
	return dev_read_mask(dev, (PHY_STAT0), PHY_STAT0_TX_PHY_LOCK_MASK);
}

static void phy_interrupt_mask(hdmi_tx_dev_t *dev, u8 mask)
{
	LOG_TRACE1(mask);
	/* Mask will determine which bits will be enabled */
	dev_write_mask(dev, PHY_MASK0, mask, 0xff);
}

#if 0
static void phy_interrupt_unmask(hdmi_tx_dev_t *dev, u8 mask)
{
	LOG_TRACE1(mask);
	/* Mask will determine which bits will be enabled */
	dev_write_mask(dev, PHY_MASK0, mask, 0x0);
}
#endif

u8 phy_hot_plug_state(hdmi_tx_dev_t *dev)
{
	return dev_read_mask(dev, (PHY_STAT0), PHY_STAT0_HPD_MASK);
}

u8 phy_rxsense_state(hdmi_tx_dev_t *dev)
{
	LOG_TRACE();
	return (u8)(dev_read_mask(dev, (PHY_STAT0), PHY_STAT0_RX_SENSE_0_MASK) |
		dev_read_mask(dev, (PHY_STAT0), PHY_STAT0_RX_SENSE_1_MASK) |
		dev_read_mask(dev, (PHY_STAT0), PHY_STAT0_RX_SENSE_2_MASK) |
		dev_read_mask(dev, (PHY_STAT0), PHY_STAT0_RX_SENSE_3_MASK));
}
u8 phy_pll_lock_state(hdmi_tx_dev_t *dev)
{
	LOG_TRACE();
	return dev_read_mask(dev, (PHY_STAT0), PHY_STAT0_TX_PHY_LOCK_MASK);
}

u8 phy_power_state(hdmi_tx_dev_t *dev)
{
	LOG_TRACE();
	return dev_read_mask(dev, PHY_CONF0, PHY_CONF0_TXPWRON_MASK);
}
void phy_power_enable(hdmi_tx_dev_t *dev, u8 enable)
{
	dev_write_mask(dev, PHY_CONF0, PHY_CONF0_PDDQ_MASK, !enable);
	dev_write_mask(dev, PHY_CONF0, PHY_CONF0_TXPWRON_MASK, enable);

}


