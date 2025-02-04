/*
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include "access.h"
#include "core/irq.h"

#define I2CM_OPERATION_READ		0x01
#define I2CM_OPERATION_READ_EXT		0x02
#define I2CM_OPERATION_READ_SEQ		0x04
#define I2CM_OPERATION_READ_SEQ_EXT     0x08
#define I2CM_OPERATION_WRITE		0x10

#define I2C_DIV_FACTOR	 100000


static struct device_access *device_bsp;
struct system_functions *snps_functions;

/* calculate valid bit account */
static u8 lowestBitSet(u8 x)
{
	u8 result = 0;

	/* x=0 is not properly handled by while-loop */
	if (x == 0)
		return 0;

	while ((x & 1) == 0) {
		x >>= 1;
		result++;
	}

	return result;
}

void register_bsp_functions(struct device_access *device)
{
	LOG_TRACE();
	device_bsp = device;
}

/**
 *Initialize communications with development board
 *@param baseAddr pointer to the address of the core on the bus
 *@return TRUE  if successful.
 */
static int dev_initialize(hdmi_tx_dev_t *dev)
{
	if (device_bsp)
		return device_bsp->initialize();

	pr_err("Error:BSP functions not registered\n");
	return -1;
}

u32 dev_read(hdmi_tx_dev_t *dev, u32 addr)
{
	if (dev && device_bsp && device_bsp->read)
		return device_bsp->read((uintptr_t)addr);

	pr_err("dev_read Error:BSP functions not registered\n");
	return 0;
}

void dev_write(hdmi_tx_dev_t *dev, u32 addr, u32 data)
{
	if (dev && device_bsp && device_bsp->write)
		return device_bsp->write((uintptr_t)addr, data);

	pr_err("dev_read Error: BSP functions not registered\n");
}

u32 dev_read_mask(hdmi_tx_dev_t *dev, u32 addr, u8 mask)
{
	u8 shift = lowestBitSet(mask);

	return (dev_read(dev, addr) & mask) >> shift;
}

void dev_write_mask(hdmi_tx_dev_t *dev, u32 addr, u8 mask, u8 data)
{
	u8 temp = 0;
	u8 shift = lowestBitSet(mask);

	temp = dev_read(dev, addr);
	temp &= ~(mask);
	temp |= (mask & (data << shift));
	dev_write(dev, addr, temp);
}


int access_Initialize(hdmi_tx_dev_t *dev)
{
	return dev_initialize(dev);
}

void register_system_functions(struct system_functions *functions)
{
	snps_functions = functions;
}

void snps_sleep(unsigned us)
{
	snps_functions->sleep(us);
}

/**
 * calculate the fast sped high time counter - round up
 */
static u16 _scl_calc(u16 sfrClock, u16 sclMinTime)
{
	unsigned long tmp_scl_period = 0;

	if (((sfrClock * sclMinTime) % I2C_DIV_FACTOR) != 0) {
		tmp_scl_period = (unsigned long)((sfrClock * sclMinTime)
			+ (I2C_DIV_FACTOR
			- ((sfrClock * sclMinTime) % I2C_DIV_FACTOR)))
			/ I2C_DIV_FACTOR;
	} else {
		tmp_scl_period = (unsigned long)(sfrClock * sclMinTime)
							/ I2C_DIV_FACTOR;
	}
	return (u16)(tmp_scl_period);
}

static void _fast_speed_high_clk_ctrl(hdmi_tx_dev_t *dev, u16 value)
{
	LOG_TRACE2(I2CM_FS_SCL_HCNT_1_ADDR, value);

	dev_write(dev, I2CM_FS_SCL_HCNT_1_ADDR, (u8) (value >> 8));
	dev_write(dev, I2CM_FS_SCL_HCNT_0_ADDR, (u8) (value >> 0));
}

static void _fast_speed_low_clk_ctrl(hdmi_tx_dev_t *dev, u16 value)
{
	LOG_TRACE2((I2CM_FS_SCL_LCNT_1_ADDR), value);
	dev_write(dev, I2CM_FS_SCL_LCNT_1_ADDR, (u8) (value >> 8));
	dev_write(dev, I2CM_FS_SCL_LCNT_0_ADDR, (u8) (value >> 0));
}

static void _standard_speed_high_clk_ctrl(hdmi_tx_dev_t *dev, u16 value)
{
	LOG_TRACE2((I2CM_SS_SCL_HCNT_1_ADDR), value);
	dev_write(dev, I2CM_SS_SCL_HCNT_1_ADDR, (u8) (value >> 8));
	dev_write(dev, I2CM_SS_SCL_HCNT_0_ADDR, (u8) (value >> 0));
}

static void _standard_speed_low_clk_ctrl(hdmi_tx_dev_t *dev, u16 value)
{
	LOG_TRACE2((I2CM_SS_SCL_LCNT_1_ADDR), value);
	dev_write(dev, I2CM_SS_SCL_LCNT_1_ADDR, (u8) (value >> 8));
	dev_write(dev, I2CM_SS_SCL_LCNT_0_ADDR, (u8) (value >> 0));
}

static int _write(hdmi_tx_dev_t *dev, u8 i2cAddr, u8 addr, u8 data)
{
	int timeout = I2CDDC_TIMEOUT;
	u32 status = 0;

	LOG_TRACE1(addr);

	dev_write_mask(dev, I2CM_SLAVE, I2CM_SLAVE_SLAVEADDR_MASK, i2cAddr);
	dev_write(dev, I2CM_ADDRESS, addr);
	dev_write(dev, I2CM_DATAO, data);
	dev_write(dev, I2CM_OPERATION, I2CM_OPERATION_WRITE);
	do {
		snps_sleep(10);
		status = dev_read_mask(dev, IH_I2CM_STAT0,
				IH_I2CM_STAT0_I2CMASTERERROR_MASK
				| IH_I2CM_STAT0_I2CMASTERDONE_MASK);
	} while (status == 0 && (timeout--));

	dev_write(dev, IH_I2CM_STAT0, status); /* clear read status */

	if (status & IH_I2CM_STAT0_I2CMASTERERROR_MASK) {
		pr_err("I2C DDC write failed");
		return -1;
	}

	if (status & IH_I2CM_STAT0_I2CMASTERDONE_MASK)
		return 0;

	pr_err("%s-Error:ASSERT I2C Write timeout - check system - exiting",
				__func__);
	return -1;
}

static int _read(hdmi_tx_dev_t *dev, u8 i2cAddr, u8 segment,
					u8 pointer, u8 addr, u8 *value)
{
	int timeout = I2CDDC_TIMEOUT;
	u32 status = 0;

	LOG_TRACE1(addr);
	dev_write_mask(dev, I2CM_SLAVE, I2CM_SLAVE_SLAVEADDR_MASK, i2cAddr);
	dev_write(dev, I2CM_ADDRESS, addr);
	dev_write(dev, I2CM_SEGADDR, segment);
	dev_write(dev, I2CM_SEGPTR, pointer);

	if (pointer)
		dev_write(dev, I2CM_OPERATION, I2CM_OPERATION_READ_EXT);
	else
		dev_write(dev, I2CM_OPERATION, I2CM_OPERATION_READ);

	do {
		snps_sleep(10);
		status = dev_read_mask(dev, IH_I2CM_STAT0,
					IH_I2CM_STAT0_I2CMASTERERROR_MASK
					| IH_I2CM_STAT0_I2CMASTERDONE_MASK);
	} while (status == 0 && (timeout--));

	dev_write(dev, IH_I2CM_STAT0, status); /* clear read status */

	if (status & IH_I2CM_STAT0_I2CMASTERERROR_MASK) {
		pr_err("I2C DDC Read failed for i2cAddr 0x%x seg 0x%x pointer 0x%x addr 0x%x",
					i2cAddr, segment, pointer, addr);
		return -1;
	}

	if (status & IH_I2CM_STAT0_I2CMASTERDONE_MASK) {
		*value = (u8) dev_read(dev, I2CM_DATAI);
		return 0;
	}

	pr_err("%s-Error:ASSERT I2C DDC Read timeout - check system - exiting",
				__func__);
	return -1;
}

static int _read8(hdmi_tx_dev_t *dev, u8 i2cAddr, u8 segment,
					u8 pointer, u8 addr, u8 *value)
{
	int timeout = I2CDDC_TIMEOUT;
	u32 status = 0;

	LOG_TRACE1(addr);
	dev_write_mask(dev, I2CM_SLAVE, I2CM_SLAVE_SLAVEADDR_MASK, i2cAddr);
	dev_write(dev, I2CM_SEGADDR, segment);
	dev_write(dev, I2CM_SEGPTR, pointer);
	dev_write(dev, I2CM_ADDRESS, addr);

	if (pointer)
		dev_write(dev, I2CM_OPERATION, I2CM_OPERATION_READ_SEQ_EXT);
	else
		dev_write(dev, I2CM_OPERATION, I2CM_OPERATION_READ_SEQ);

	do {
		snps_sleep(10);
		status = dev_read_mask(dev, IH_I2CM_STAT0,
				IH_I2CM_STAT0_I2CMASTERERROR_MASK |
				IH_I2CM_STAT0_I2CMASTERDONE_MASK);
	} while (status == 0 && (timeout--));

	dev_write(dev, IH_I2CM_STAT0, status); /* clear read status */

	if (status & IH_I2CM_STAT0_I2CMASTERERROR_MASK) {
		pr_err("%s-Error:I2C DDC Read8 extended failed for i2cAddr 0x%x seg 0x%x pointer 0x%x addr 0x%x\n",
				__func__, i2cAddr, segment, pointer, addr);
		return -1;
	}

	if (status & IH_I2CM_STAT0_I2CMASTERDONE_MASK) {
		int i = 0;

		while (i < 8) { /* read 8 bytes */
			value[i] = (u8) dev_read(dev,
					I2CM_READ_BUFF0 + (4 * i));
			i += 1;
		}
		return 0;
	}

	pr_err("%s-Error:ASSERT I2C DDC Read extended timeout - check system - exiting\n",
				__func__);
	return -1;
}

/* ********************  PUBLIC FUNCTIONS ********************** */

void i2cddc_clk_config(hdmi_tx_dev_t *dev, u16 sfrClock,
	u16 ss_low_ckl, u16 ss_high_ckl, u16 fs_low_ckl, u16 fs_high_ckl)
{
	_standard_speed_low_clk_ctrl(dev, _scl_calc(sfrClock, ss_low_ckl));
	_standard_speed_high_clk_ctrl(dev, _scl_calc(sfrClock, ss_high_ckl));
	_fast_speed_low_clk_ctrl(dev, _scl_calc(sfrClock, fs_low_ckl));
	_fast_speed_high_clk_ctrl(dev, _scl_calc(sfrClock, fs_high_ckl));
}

void i2cddc_fast_mode(hdmi_tx_dev_t *dev, u8 value)
{
	LOG_TRACE1(value);
	/* bit 4 selects between high and standard speed operation */
	dev_write_mask(dev, I2CM_DIV, I2CM_DIV_FAST_STD_MODE_MASK, value);
}


void i2cddc_mask_interrupts(hdmi_tx_dev_t *dev, u8 mask)
{
	LOG_TRACE1(mask);

	dev_write_mask(dev, I2CM_INT, I2CM_INT_DONE_MASK, mask ? 1 : 0);
	dev_write_mask(dev, I2CM_CTLINT,
				I2CM_CTLINT_ARBITRATION_MASK, mask ? 1 : 0);
	dev_write_mask(dev, I2CM_CTLINT, I2CM_CTLINT_NACK_MASK, mask ? 1 : 0);
}


int ddc_write(hdmi_tx_dev_t *dev, u8 i2cAddr, u8 addr, u8 len, u8 *data)
{
	int i, status = 0;

	for (i = 0; i < len; i++) {
		int tries = 3;

		do {
			status = _write(dev, i2cAddr, addr, data[i]);
		} while (status && tries--);

		if (status) /* Error after 3 failed writes */
			return status;
	}
	return 0;
}

int ddc_read(hdmi_tx_dev_t *dev, u8 i2cAddr, u8 segment,
				u8 pointer, u8 addr, u8 len, u8 *data)
{
	int i, status = 0;

	for (i = 0; i < len;) {
		int tries = 3;

		if ((len - i) >= 8) {
			do {
				status = _read8(dev, i2cAddr, segment,
						pointer, addr + i,  &(data[i]));
			} while (status && tries--);

			if (status) /* Error after 3 failed writes */
				return status;
			i += 8;
		} else {
			do {
				status = _read(dev, i2cAddr, segment,
						pointer, addr + i,  &(data[i]));
			} while (status && tries--);

			if (status) /* Error after 3 failed writes */
				return status;
			i++;
		}
	}
	return 0;
}

#ifdef CONFIG_AW_HDMI2_HDCP_SUNXI
int ddc_read_hdcp2Version(hdmi_tx_dev_t *dev, u8 *data)
{
	if (ddc_read(dev, 0x3A, 0, 0, 0x50, 1, data)) {
		pr_info("%s: DDC Read hdcp2Version fail!\n", __func__);
		return -1;
	}
	return 0;
}
#endif
