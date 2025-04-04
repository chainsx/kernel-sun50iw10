/*
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include "scdc.h"

#ifndef SUPPORT_ONLY_HDMI14
/* static void scdc_enable_rr(hdmi_tx_dev_t *dev, u8 enable); */
/* static int scdc_scrambling_status(hdmi_tx_dev_t *dev); */
static int scdc_scrambling_enable_flag(hdmi_tx_dev_t *dev, u8 flag);
/* static void scdc_set_rr_flag(hdmi_tx_dev_t *dev, u8 enable); */
/* static int scdc_get_rr_flag(hdmi_tx_dev_t *dev, u8 *flag); */
static void scrambling_Enable(hdmi_tx_dev_t *dev, u8 bit);

int scdc_read(hdmi_tx_dev_t *dev, u8 address, u8 size, u8 *data)
{
	if (ddc_read(dev, SCDC_SLAVE_ADDRESS, 0, 0, address, size, data)) {
		pr_err("Error:SCDC addr 0x%x read failed\n",
							address);
		return -1;
	}
	return 0;
}

int scdc_write(hdmi_tx_dev_t *dev, u8 address, u8 size, u8 *data)
{
	if (ddc_write(dev, SCDC_SLAVE_ADDRESS, address, size, data)) {
		pr_err("Error:SCDC addr 0x%x write failed\n",
							address);
		return -1;
	}
	return 0;
}

void scrambling(hdmi_tx_dev_t *dev, u8 enable)
{
	u8 data = 0;

	LOG_TRACE1(enable);
	if (enable == 1) {
		scdc_scrambling_enable_flag(dev, 1);

		/* Start/stop HDCP keep-out window generation not needed because it's always on */
		/* TMDS software reset request */
		mc_tmds_clock_reset(dev, TRUE);

		/* Enable/Disable Scrambling */
		scrambling_Enable(dev, TRUE);

		if (scdc_read(dev, SCDC_TMDS_CONFIG, 1, &data))
			pr_err("%s: SCDC addr 0x%x read failed ", __func__, SCDC_TMDS_CONFIG);
		/* Write on RX the bit Scrambling_Enable, register 0x20 */
		data |= 0x02;
		scdc_write(dev, SCDC_TMDS_CONFIG, 1, &data);
		data |= 0x01;
		scdc_write(dev, SCDC_TMDS_CONFIG, 1, &data);
	} else {
		/* Enable/Disable Scrambling */
		scrambling_Enable(dev, FALSE);
		scdc_scrambling_enable_flag(dev, 0);

		/* TMDS software reset request */
		mc_tmds_clock_reset(dev, TRUE);

		/* Write on RX the bit Scrambling_Enable, register 0x20 */
		data = 0x00;
		scdc_write(dev, SCDC_TMDS_CONFIG, 1, &data);
	}
}

void scrambling_Enable(hdmi_tx_dev_t *dev, u8 bit)
{
	dev_write_mask(dev, FC_SCRAMBLER_CTRL,
			FC_SCRAMBLER_CTRL_SCRAMBLER_ON_MASK, bit);
}

/* static void scdc_enable_rr(hdmi_tx_dev_t *dev, u8 enable)
{

	if (enable == 1) { */
		/* Enable Readrequest from the Tx controller */
		/* dev_write_mask(dev, I2CM_SCDC_READ_UPDATE,
				I2CM_SCDC_READ_UPDATE_READ_REQUEST_EN_MASK, 1);
		scdc_set_rr_flag(dev, 0x01);
		irq_scdc_read_request(dev, TRUE);
	} */
	/* if (enable == 0) { */
		/* Disable ReadRequest on Tx controller */
		/* irq_scdc_read_request(dev, FALSE);
		scdc_set_rr_flag(dev, 0x00);
		dev_write_mask(dev, I2CM_SCDC_READ_UPDATE,
			I2CM_SCDC_READ_UPDATE_READ_REQUEST_EN_MASK, 0);
	}
} */

/* static int scdc_scrambling_status(hdmi_tx_dev_t *dev)
{
	u8 read_value = 0;

	if (scdc_read(dev, SCDC_SCRAMBLER_STAT, 1, &read_value)) {
		HDMI_ERROR_MSG("SCDC addr 0x%x read failed\n",
						SCDC_SINK_VER);
		return 0;
	}
	return read_value & 0x01;
} */

static int scdc_scrambling_enable_flag(hdmi_tx_dev_t *dev, u8 enable)
{
	u8 read_value = 0;

	if (scdc_read(dev, SCDC_TMDS_CONFIG, 1, &read_value)) {
		pr_err("Error:SCDC addr 0x%x read failed\n",
						SCDC_TMDS_CONFIG);
		return -1;
	}
	read_value = set(read_value, 0x1, enable ? 0x1 : 0x0);
	if (scdc_write(dev, SCDC_TMDS_CONFIG, 1, &read_value)) {
		pr_err("Error:SCDC addr 0x%x write failed\n",
						SCDC_TMDS_CONFIG);
		return -1;
	}
	return 0;
}

/* static void scdc_set_rr_flag(hdmi_tx_dev_t *dev, u8 enable)
{

	if (ddc_write(dev, SCDC_SLAVE_ADDRESS, SCDC_CONFIG_0, 1, &enable)) {
		HDMI_ERROR_MSG("SCDC addr 0x%x - 0x%x write failed\n",
					SCDC_CONFIG_0, enable);
	}
} */

/* static int scdc_get_rr_flag(hdmi_tx_dev_t *dev, u8 *flag)
{
	if (ddc_read(dev, SCDC_SLAVE_ADDRESS, 0, 0, SCDC_CONFIG_0, 1, flag)) {
		HDMI_ERROR_MSG("SCDC addr 0x%x read failed\n",
						SCDC_CONFIG_0);
		return -1;
	}
	return 0;
} */

/* static void scdc_test_rr(hdmi_tx_dev_t *dev, u8 test_rr_delay)
{
	scdc_enable_rr(dev, 0x01);
	test_rr_delay = set(test_rr_delay, 0x80, 1);
	scdc_write(dev, SCDC_TEST_CFG_0, 1, &test_rr_delay);
} */

/* static int scdc_test_rr_update_flag(hdmi_tx_dev_t *dev)
{
	u8 read_value = 0;

	if (scdc_read(dev, SCDC_UPDATE_0, 1, &read_value)) {
		HDMI_ERROR_MSG("SCDC addr 0x%x read failed\n",
						SCDC_UPDATE_0);
		return 0;
	}
	return read_value;
} */

u8 scrambling_state(hdmi_tx_dev_t *dev)
{
	return dev_read_mask(dev, FC_SCRAMBLER_CTRL,
			FC_SCRAMBLER_CTRL_SCRAMBLER_ON_MASK);
}
#endif
