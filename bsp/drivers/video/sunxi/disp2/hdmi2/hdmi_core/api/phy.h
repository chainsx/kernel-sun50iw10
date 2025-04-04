/*
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef PHY_H_
#define PHY_H_

#include "hdmitx_dev.h"
#include "core/video.h"
#include "core/main_controller.h"
#include "log.h"
#include "access.h"

#define PHY_TIMEOUT			100
#define PHY_I2C_SLAVE_ADDR		0x69

#define PHY_MODEL_301		301
#define PHY_MODEL_303		303
#define PHY_MODEL_108		108

#define OPMODE_PLLCFG 0x06 /* Mode of Operation and PLL  Dividers Control Register */
#define CKSYMTXCTRL   0x09 /* Clock Symbol and Transmitter Control Register */
#define PLLCURRCTRL   0x10 /* PLL Current Control Register */
#define VLEVCTRL      0x0E /* Voltage Level Control Register */
#define PLLGMPCTRL    0x15 /* PLL Gmp Control Register */
#define TXTERM        0x19 /* Transmission Termination Register */

#define JTAG_TAP_ADDR_CMD	0
#define JTAG_TAP_WRITE_CMD	1
#define JTAG_TAP_READ_CMD	3

/*****************************************************************************
 *                                                                           *
 *                        PHY Configuration Registers                        *
 *                                                                           *
 *****************************************************************************/

/* PHY Configuration Register This register holds the power down, data enable polarity, and interface control of the HDMI Source PHY control */
#define PHY_CONF0  0x0000C000
#define PHY_CONF0_SELDIPIF_MASK  0x00000001 /* Select interface control */
#define PHY_CONF0_SELDATAENPOL_MASK  0x00000002 /* Select data enable polarity */
#define PHY_CONF0_ENHPDRXSENSE_MASK  0x00000004 /* PHY ENHPDRXSENSE signal */
#define PHY_CONF0_TXPWRON_MASK  0x00000008 /* PHY TXPWRON signal */
#define PHY_CONF0_PDDQ_MASK  0x00000010 /* PHY PDDQ signal */
#define PHY_CONF0_SVSRET_MASK  0x00000020 /* Reserved as "spare" register with no associated functionality */
#define PHY_CONF0_SPARES_1_MASK  0x00000040 /* Reserved as "spare" register with no associated functionality */
#define PHY_CONF0_SPARES_2_MASK  0x00000080 /* Reserved as "spare" register with no associated functionality */

/* PHY Test Interface Register 0 PHY TX mapped test interface (control) */
#define PHY_TST0  0x0000C004
#define PHY_TST0_SPARE_0_MASK  0x00000001 /* Reserved as "spare" register with no associated functionality */
#define PHY_TST0_SPARE_1_MASK  0x0000000E /* Reserved as "spare" bit with no associated functionality */
#define PHY_TST0_SPARE_3_MASK  0x00000010 /* Reserved as "spare" register with no associated functionality */
#define PHY_TST0_SPARE_4_MASK  0x00000020 /* Reserved as "spare" register with no associated functionality */
#define PHY_TST0_SPARE_2_MASK  0x000000C0 /* Reserved as "spare" bit with no associated functionality */

/* PHY Test Interface Register 1 PHY TX mapped text interface (data in) */
#define PHY_TST1  0x0000C008
#define PHY_TST1_SPARE_MASK  0x000000FF /* Reserved as "spare" register with no associated functionality */

/* PHY Test Interface Register 2 PHY TX mapped text interface (data out) */
#define PHY_TST2  0x0000C00C
#define PHY_TST2_SPARE_MASK  0x000000FF /* Reserved as "spare" register with no associated functionality */

/* PHY RXSENSE, PLL Lock, and HPD Status Register This register contains the following active high packet sent status indications */
#define PHY_STAT0  0x0000C010
#define PHY_STAT0_TX_PHY_LOCK_MASK  0x00000001 /* Status bit */
#define PHY_STAT0_HPD_MASK  0x00000002 /* Status bit */
#define PHY_STAT0_RX_SENSE_0_MASK  0x00000010 /* Status bit */
#define PHY_STAT0_RX_SENSE_1_MASK  0x00000020 /* Status bit */
#define PHY_STAT0_RX_SENSE_2_MASK  0x00000040 /* Status bit */
#define PHY_STAT0_RX_SENSE_3_MASK  0x00000080 /* Status bit */

/* PHY RXSENSE, PLL Lock, and HPD Interrupt Register This register contains the interrupt indication of the PHY_STAT0 status interrupts */
#define PHY_INT0  0x0000C014
#define PHY_INT0_TX_PHY_LOCK_MASK  0x00000001 /* Interrupt indication bit */
#define PHY_INT0_HPD_MASK  0x00000002 /* Interrupt indication bit */
#define PHY_INT0_RX_SENSE_0_MASK  0x00000010 /* Interrupt indication bit */
#define PHY_INT0_RX_SENSE_1_MASK  0x00000020 /* Interrupt indication bit */
#define PHY_INT0_RX_SENSE_2_MASK  0x00000040 /* Interrupt indication bit */
#define PHY_INT0_RX_SENSE_3_MASK  0x00000080 /* Interrupt indication bit */

/* PHY RXSENSE, PLL Lock, and HPD Mask Register Mask register for generation of PHY_INT0 interrupts */
#define PHY_MASK0  0x0000C018
#define PHY_MASK0_TX_PHY_LOCK_MASK  0x00000001 /* Mask bit for PHY_INT0 */
#define PHY_MASK0_HPD_MASK  0x00000002 /* Mask bit for PHY_INT0 */
#define PHY_MASK0_RX_SENSE_0_MASK  0x00000010 /* Mask bit for PHY_INT0 */
#define PHY_MASK0_RX_SENSE_1_MASK  0x00000020 /* Mask bit for PHY_INT0 */
#define PHY_MASK0_RX_SENSE_2_MASK  0x00000040 /* Mask bit for PHY_INT0 */
#define PHY_MASK0_RX_SENSE_3_MASK  0x00000080 /* Mask bit for PHY_INT0 */

/* PHY RXSENSE, PLL Lock, and HPD Polarity Register Polarity register for generation of PHY_INT0 interrupts */
#define PHY_POL0  0x0000C01C
#define PHY_POL0_TX_PHY_LOCK_MASK  0x00000001 /* Polarity bit for PHY_INT0 */
#define PHY_POL0_HPD_MASK  0x00000002 /* Polarity bit for PHY_INT0 */
#define PHY_POL0_RX_SENSE_0_MASK  0x00000010 /* Polarity bit for PHY_INT0 */
#define PHY_POL0_RX_SENSE_1_MASK  0x00000020 /* Polarity bit for PHY_INT0 */
#define PHY_POL0_RX_SENSE_2_MASK  0x00000040 /* Polarity bit for PHY_INT0 */
#define PHY_POL0_RX_SENSE_3_MASK  0x00000080 /* Polarity bit for PHY_INT0 */

/* PHY I2C Slave Address Configuration Register */
#define PHY_I2CM_SLAVE  0x0000C080
#define PHY_I2CM_SLAVE_SLAVEADDR_MASK  0x0000007F /* Slave address to be sent during read and write operations */

/* PHY I2C Address Configuration Register This register writes the address for read and write operations */
#define PHY_I2CM_ADDRESS  0x0000C084
#define PHY_I2CM_ADDRESS_ADDRESS_MASK  0x000000FF /* Register address for read and write operations */

/* PHY I2C Data Write Register 1 */
#define PHY_I2CM_DATAO_1  0x0000C088
#define PHY_I2CM_DATAO_1_DATAO_MASK  0x000000FF /* Data MSB (datao[15:8]) to be written on register pointed by phy_i2cm_address [7:0] */

/* PHY I2C Data Write Register 0 */
#define PHY_I2CM_DATAO_0  0x0000C08C
#define PHY_I2CM_DATAO_0_DATAO_MASK  0x000000FF /* Data LSB (datao[7:0]) to be written on register pointed by phy_i2cm_address [7:0] */

/* PHY I2C Data Read Register 1 */
#define PHY_I2CM_DATAI_1  0x0000C090
#define PHY_I2CM_DATAI_1_DATAI_MASK  0x000000FF /* Data MSB (datai[15:8]) read from register pointed by phy_i2cm_address[7:0] */

/* PHY I2C Data Read Register 0 */
#define PHY_I2CM_DATAI_0  0x0000C094
#define PHY_I2CM_DATAI_0_DATAI_MASK  0x000000FF /* Data LSB (datai[7:0]) read from register pointed by phy_i2cm_address[7:0] */

/* PHY I2C RD/RD_EXT/WR Operation Register This register requests read and write operations from the I2C Master PHY */
#define PHY_I2CM_OPERATION  0x0000C098
#define PHY_I2CM_OPERATION_RD_MASK  0x00000001 /* Read operation request */
#define PHY_I2CM_OPERATION_WR_MASK  0x00000010 /* Write operation request */

/* PHY I2C Done Interrupt Register This register contains and configures I2C master PHY done interrupt */
#define PHY_I2CM_INT  0x0000C09C
#define PHY_I2CM_INT_DONE_STATUS_MASK  0x00000001 /* Operation done status bit */
#define PHY_I2CM_INT_DONE_INTERRUPT_MASK  0x00000002 /* Operation done interrupt bit */
#define PHY_I2CM_INT_DONE_MASK_MASK  0x00000004 /* Done interrupt mask signal */
#define PHY_I2CM_INT_DONE_POL_MASK  0x00000008 /* Done interrupt polarity configuration */

/* PHY I2C error Interrupt Register This register contains and configures the I2C master PHY error interrupts */
#define PHY_I2CM_CTLINT  0x0000C0A0
#define PHY_I2CM_CTLINT_ARBITRATION_STATUS_MASK  0x00000001 /* Arbitration error status bit */
#define PHY_I2CM_CTLINT_ARBITRATION_INTERRUPT_MASK  0x00000002 /* Arbitration error interrupt bit {arbitration_interrupt = (arbitration_mask==0b) && (arbitration_status==arbitration_pol)} Note: This bit field is read by the sticky bits present on the ih_i2cmphy_stat0 register */
#define PHY_I2CM_CTLINT_ARBITRATION_MASK_MASK  0x00000004 /* Arbitration error interrupt mask signal */
#define PHY_I2CM_CTLINT_ARBITRATION_POL_MASK  0x00000008 /* Arbitration error interrupt polarity configuration */
#define PHY_I2CM_CTLINT_NACK_STATUS_MASK  0x00000010 /* Not acknowledge error status bit */
#define PHY_I2CM_CTLINT_NACK_INTERRUPT_MASK  0x00000020 /* Not acknowledge error interrupt bit */
#define PHY_I2CM_CTLINT_NACK_MASK_MASK  0x00000040 /* Not acknowledge error interrupt mask signal */
#define PHY_I2CM_CTLINT_NACK_POL_MASK  0x00000080 /* Not acknowledge error interrupt polarity configuration */

/* PHY I2C Speed control Register This register wets the I2C Master PHY to work in either Fast or Standard mode */
#define PHY_I2CM_DIV  0x0000C0A4
#define PHY_I2CM_DIV_SPARE_MASK  0x00000007 /* Reserved as "spare" register with no associated functionality */
#define PHY_I2CM_DIV_FAST_STD_MODE_MASK  0x00000008 /* Sets the I2C Master to work in Fast Mode or Standard Mode: 1: Fast Mode 0: Standard Mode */

/* PHY I2C SW reset control register This register sets the I2C Master PHY software reset */
#define PHY_I2CM_SOFTRSTZ  0x0000C0A8
#define PHY_I2CM_SOFTRSTZ_I2C_SOFTRSTZ_MASK  0x00000001 /* I2C Master Software Reset */

/* PHY I2C Slow Speed SCL High Level Control Register 1 */
#define PHY_I2CM_SS_SCL_HCNT_1_ADDR  0x0000C0AC
#define PHY_I2CM_SS_SCL_HCNT_1_ADDR_I2CMP_SS_SCL_HCNT1_MASK  0x000000FF /* PHY I2C Slow Speed SCL High Level Control Register 1 */

/* PHY I2C Slow Speed SCL High Level Control Register 0 */
#define PHY_I2CM_SS_SCL_HCNT_0_ADDR  0x0000C0B0
#define PHY_I2CM_SS_SCL_HCNT_0_ADDR_I2CMP_SS_SCL_HCNT0_MASK  0x000000FF /* PHY I2C Slow Speed SCL High Level Control Register 0 */

/* PHY I2C Slow Speed SCL Low Level Control Register 1 */
#define PHY_I2CM_SS_SCL_LCNT_1_ADDR  0x0000C0B4
#define PHY_I2CM_SS_SCL_LCNT_1_ADDR_I2CMP_SS_SCL_LCNT1_MASK  0x000000FF /* PHY I2C Slow Speed SCL Low Level Control Register 1 */

/* PHY I2C Slow Speed SCL Low Level Control Register 0 */
#define PHY_I2CM_SS_SCL_LCNT_0_ADDR  0x0000C0B8
#define PHY_I2CM_SS_SCL_LCNT_0_ADDR_I2CMP_SS_SCL_LCNT0_MASK  0x000000FF /* PHY I2C Slow Speed SCL Low Level Control Register 0 */

/* PHY I2C Fast Speed SCL High Level Control Register 1 */
#define PHY_I2CM_FS_SCL_HCNT_1_ADDR  0x0000C0BC
#define PHY_I2CM_FS_SCL_HCNT_1_ADDR_I2CMP_FS_SCL_HCNT1_MASK  0x000000FF /* PHY I2C Fast Speed SCL High Level Control Register 1 */

/* PHY I2C Fast Speed SCL High Level Control Register 0 */
#define PHY_I2CM_FS_SCL_HCNT_0_ADDR  0x0000C0C0
#define PHY_I2CM_FS_SCL_HCNT_0_ADDR_I2CMP_FS_SCL_HCNT0_MASK  0x000000FF /* PHY I2C Fast Speed SCL High Level Control Register 0 */

/* PHY I2C Fast Speed SCL Low Level Control Register 1 */
#define PHY_I2CM_FS_SCL_LCNT_1_ADDR  0x0000C0C4
#define PHY_I2CM_FS_SCL_LCNT_1_ADDR_I2CMP_FS_SCL_LCNT1_MASK  0x000000FF /* PHY I2C Fast Speed SCL Low Level Control Register 1 */

/* PHY I2C Fast Speed SCL Low Level Control Register 0 */
#define PHY_I2CM_FS_SCL_LCNT_0_ADDR  0x0000C0C8
#define PHY_I2CM_FS_SCL_LCNT_0_ADDR_I2CMP_FS_SCL_LCNT0_MASK  0x000000FF /* PHY I2C Fast Speed SCL Low Level Control Register 0 */

/* PHY I2C SDA HOLD Control Register */
#define PHY_I2CM_SDA_HOLD  0x0000C0CC
#define PHY_I2CM_SDA_HOLD_OSDA_HOLD_MASK  0x000000FF /* Defines the number of SFR clock cycles to meet tHD:DAT (300 ns) osda_hold = round_to_high_integer (300 ns / (1/isfrclk_frequency)) */

/* PHY I2C/JTAG I/O Configuration Control Register */
#define JTAG_PHY_CONFIG  0x0000C0D0
#define JTAG_PHY_CONFIG_JTAG_TRST_N_MASK  0x00000001 /* Configures the JTAG PHY interface output pin JTAG_TRST_N when in internal control mode (iphy_ext_ctrl=1'b0) or ophyext_jtag_trst_n when PHY_EXTERNAL=1 */
#define JTAG_PHY_CONFIG_I2C_JTAGZ_MASK  0x00000010 /* Configures the JTAG PHY interface output pin I2C_JTAGZ to select the PHY configuration interface when in internal control mode (iphy_ext_ctrl=1'b0) or ophyext_jtag_i2c_jtagz when PHY_EXTERNAL=1 */

/* PHY JTAG Clock Control Register */
#define JTAG_PHY_TAP_TCK  0x0000C0D4
#define JTAG_PHY_TAP_TCK_JTAG_TCK_MASK  0x00000001 /* Configures the JTAG PHY interface pin JTAG_TCK when in internal control mode (iphy_ext_ctrl=1'b0) or ophyext_jtag_tck when PHY_EXTERNAL=1 */

/* PHY JTAG TAP In Control Register */
#define JTAG_PHY_TAP_IN  0x0000C0D8
#define JTAG_PHY_TAP_IN_JTAG_TDI_MASK  0x00000001 /* Configures the JTAG PHY interface pin JTAG_TDI when in internal control mode (iphy_ext_ctrl=1'b0) or ophyext_jtag_tdi when PHY_EXTERNAL=1 */
#define JTAG_PHY_TAP_IN_JTAG_TMS_MASK  0x00000010 /* Configures the JTAG PHY interface pin JTAG_TMS when in internal control mode (iphy_ext_ctrl=1'b0) or ophyext_jtag_tms when PHY_EXTERNAL=1 */

/* PHY JTAG TAP Out Control Register */
#define JTAG_PHY_TAP_OUT  0x0000C0DC
#define JTAG_PHY_TAP_OUT_JTAG_TDO_MASK  0x00000001 /* Read JTAG PHY interface input pin JTAG_TDO when in internal control mode (iphy_ext_ctrl=1'b0) or iphyext_jtag_tdo when PHY_EXTERNAL=1 */
#define JTAG_PHY_TAP_OUT_JTAG_TDO_EN_MASK  0x00000010 /* Read JTAG PHY interface input pin JTAG_TDO_EN when in internal control mode (iphy_ext_ctrl=1'b0) or iphyext_jtag_tdo_en when PHY_EXTERNAL=1 */

/* PHY JTAG Address Control Register */
#define JTAG_PHY_ADDR  0x0000C0E0
#define JTAG_PHY_ADDR_JTAG_ADDR_MASK  0x000000FF /* Configures the JTAG PHY interface pin JTAG_ADDR[7:0] when in internal control mode (iphy_ext_ctrl=1'b0) or iphyext_jtag_addr[7:0] when PHY_EXTERNAL=1 */


struct phy_config {
	u32			clock;/* phy clock: unit:kHZ */
	pixel_repetition_t	pixel;
	color_depth_t		color;
	operation_mode_t        opmode;
	u16			oppllcfg;
	u16			pllcurrctrl;
	u16			pllgmpctrl;
	u16                     txterm;
	u16                     vlevctrl;
	u16                     cksymtxctrl;
};

struct phy_config303 {
	u32			clock;
	pixel_repetition_t	pixel;
	color_depth_t		color;
	operation_mode_t        opmode;
	u16			oppllcfg;
	u16			pllcurrctrl;
	u16			pllgmpctrl;
	u16                     txterm;
	u16                     vlevctrl;
	u16                     cksymtxctrl;
};


/**
 * Initialize PHY and put into a known state
 * @param dev device structure
 * @param phy_model
 * return always TRUE
 */
int phy_initialize(hdmi_tx_dev_t *dev, u16 phy_model);

/**
 * Bring up PHY and start sending media for a specified pixel clock, pixel
 * repetition and color resolution (to calculate TMDS) - this fields must
 * be configured in the dev->snps_hdmi_ctrl variables
 * @param dev device structure
 * return TRUE if success, FALSE if not success and -1 if PHY configurations
 * are not supported.
 */
int phy_configure(hdmi_tx_dev_t *dev, u16 phy_model, encoding_t EncodingOut);

/**
 * Set PHY to standby mode - turn off all interrupts
 * @param dev device structure
 */
int phy_standby(hdmi_tx_dev_t *dev);

/**
 * Enable HPD sensing circuitry
 * @param dev device structure
 */
int phy_enable_hpd_sense(hdmi_tx_dev_t *dev);

/**
 * Disable HPD sensing circuitry
 * @param dev device structure
 */
int phy_disable_hpd_sense(hdmi_tx_dev_t *dev);

int phy_i2c_read(hdmi_tx_dev_t *dev, u8 addr, u16 *value);
int phy_i2c_write(hdmi_tx_dev_t *dev, u8 addr, u16 data);

u8 phy_hot_plug_state(hdmi_tx_dev_t *dev);

void phy_i2c_fast_mode(hdmi_tx_dev_t *dev, u8 bit);
void phy_i2c_master_reset(hdmi_tx_dev_t *dev);

u8 phy_rxsense_state(hdmi_tx_dev_t *dev);
u8 phy_pll_lock_state(hdmi_tx_dev_t *dev);
u8 phy_power_state(hdmi_tx_dev_t *dev);

void phy_power_enable(hdmi_tx_dev_t *dev, u8 enable);
void phy_set_reg_base(uintptr_t base);
uintptr_t phy_get_reg_base(void);


#endif	/* PHY_H_ */
