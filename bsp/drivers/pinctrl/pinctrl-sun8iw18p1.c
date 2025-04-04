/*
 * Allwinner sun8iw18p1 SoCs pinctrl driver.
 *
 * Copyright(c) 2017-2020 Allwinnertech Co., Ltd.
 * Author: huangshuosheng <huangshuosheng@allwinnertech.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 * */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/pinctrl/pinctrl.h>

#include "pinctrl-sunxi.h"

static const struct sunxi_desc_pin sun8iw18p1_pins[] = {
	/* HOLE */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart2"),		/* TX */
		SUNXI_FUNCTION(0x3, "pwm0"),
		SUNXI_FUNCTION(0x4, "jtag0"),		/* MS0 */
		SUNXI_FUNCTION(0x5, "ledc"),
		SUNXI_FUNCTION(0x7, "io_disabled"),
		SUNXI_FUNCTION_IRQ_BANK(0x6, 0, 0)),	/* PB_EINT0 */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart2"),		/* RX */
		SUNXI_FUNCTION(0x3, "pwm1"),
		SUNXI_FUNCTION(0x4, "jtag0"),		/* CK0 */
		SUNXI_FUNCTION(0x7, "io_disabled"),
		SUNXI_FUNCTION_IRQ_BANK(0x6, 0, 1)),	/* PB_EINT1 */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart2"),		/* SYNC */
		SUNXI_FUNCTION(0x3, "pwm2"),
		SUNXI_FUNCTION(0x4, "jtag0"),		/* DO0 */
		SUNXI_FUNCTION(0x5, "i2s0"),            /* LRCK */
		SUNXI_FUNCTION(0x7, "io_disabled"),
		SUNXI_FUNCTION_IRQ_BANK(0x6, 0, 2)),	/* PB_EINT2 */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart2"),		/* BCLK */
		SUNXI_FUNCTION(0x3, "pwm3"),
		SUNXI_FUNCTION(0x4, "jtag0"),		/* DI0 */
		SUNXI_FUNCTION(0x5, "i2s0"),            /* BCLK */
		SUNXI_FUNCTION(0x7, "io_disabled"),
		SUNXI_FUNCTION_IRQ_BANK(0x6, 0, 3)),	/* PB_EINT3 */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x3, "pwm4"),
		SUNXI_FUNCTION(0x4, "i2s0"),		/* DOUT0 */
		SUNXI_FUNCTION(0x5, "i2s0_b"),		/* DIN1 */
		SUNXI_FUNCTION(0x7, "io_disabled"),
		SUNXI_FUNCTION_IRQ_BANK(0x6, 0, 4)),	/* PB_EINT4 */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x3, "pwm5"),
		SUNXI_FUNCTION(0x4, "i2s0_b"),		/* DOUT1 */
		SUNXI_FUNCTION(0x5, "i2s0"),		/* DIN0 */
		SUNXI_FUNCTION(0x7, "io_disabled"),
		SUNXI_FUNCTION_IRQ_BANK(0x6, 0, 5)),	/* PB_EINT5 */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x3, "pwm6"),
		SUNXI_FUNCTION(0x4, "i2s0"),		/* DOUT2 */
		SUNXI_FUNCTION(0x5, "i2s0"),		/* DIN3 */
		SUNXI_FUNCTION(0x7, "io_disabled"),
		SUNXI_FUNCTION_IRQ_BANK(0x6, 0, 6)),	/* PB_EINT6 */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x3, "pwm7"),
		SUNXI_FUNCTION(0x4, "i2s0"),		/* DOUT3 */
		SUNXI_FUNCTION(0x5, "i2s0"),		/* DIN2 */
		SUNXI_FUNCTION(0x7, "io_disabled"),
		SUNXI_FUNCTION_IRQ_BANK(0x6, 0, 7)),	/* PB_EINT7 */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "i2s1"),		/* LRCK */
		SUNXI_FUNCTION(0x3, "dmic"),		/* DATA3 */
		SUNXI_FUNCTION(0x4, "twi1"),		/* SCK */
		SUNXI_FUNCTION(0x5, "uart0"),		/* TX */
		SUNXI_FUNCTION(0x7, "io_disabled"),
		SUNXI_FUNCTION_IRQ_BANK(0x6, 0, 8)),	/* PB_EINT8 */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "i2s1"),		/* BCLK */
		SUNXI_FUNCTION(0x3, "dmic"),		/* DATA2 */
		SUNXI_FUNCTION(0x4, "twi1"),		/* SDA */
		SUNXI_FUNCTION(0x5, "uart0"),		/* RX */
		SUNXI_FUNCTION(0x7, "io_disabled"),
		SUNXI_FUNCTION_IRQ_BANK(0x6, 0, 9)),	/* PB_EINT9 */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "i2s1"),		/* DOUT0 */
		SUNXI_FUNCTION(0x3, "dmic"),		/* DATA1 */
		SUNXI_FUNCTION(0x4, "i2s1_b"),		/* DIN1 */
		SUNXI_FUNCTION(0x5, "uart2"),		/* RTS */
		SUNXI_FUNCTION(0x7, "io_disabled"),
		SUNXI_FUNCTION_IRQ_BANK(0x6, 0, 10)),	/* PB_EINT10 */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "i2s1_b"),		/* DOUT1 */
		SUNXI_FUNCTION(0x3, "dmic"),		/* DATA0 */
		SUNXI_FUNCTION(0x4, "i2s1"),		/* DIN0 */
		SUNXI_FUNCTION(0x5, "uart2"),		/* CTS */
		SUNXI_FUNCTION(0x7, "io_disabled"),
		SUNXI_FUNCTION_IRQ_BANK(0x6, 0, 11)),	/* PB_EINT11 */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 12),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "i2s1"),		/* MCLK */
		SUNXI_FUNCTION(0x3, "dmic"),		/* CLK */
		SUNXI_FUNCTION(0x7, "io_disabled"),
		SUNXI_FUNCTION_IRQ_BANK(0x6, 0, 12)),	/* PB_EINT12 */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 13),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "pwm8"),
		SUNXI_FUNCTION(0x3, "ledc"),		/* DO */
		SUNXI_FUNCTION(0x5, "i2s0"),		/* MCLK */
		SUNXI_FUNCTION(0x7, "io_disabled"),
		SUNXI_FUNCTION_IRQ_BANK(0x6, 0, 13)),	/* PB_EINT13 */
	/* HOLE */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "nand0"),		/* WE */
		SUNXI_FUNCTION(0x4, "spi0"),		/* CLK */
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "nand0"),		/* ALE */
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "nand0"),		/* CLE */
		SUNXI_FUNCTION(0x4, "spi0"),		/* MOSI */
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "nand0"),		/* CE1 */
		SUNXI_FUNCTION(0x4, "spi0"),		/* CS0 */
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "nand0"),		/* CE0 */
		SUNXI_FUNCTION(0x4, "spi0"),		/* MISO */
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "nand0"),		/* RE */
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "nand0"),		/* RB0 */
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "nand0"),		/* RB1 */
		SUNXI_FUNCTION(0x4, "spi0"),		/* CS1 */
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "nand0"),		/* DQ7 */
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "nand0"),		/* DQ6 */
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "nand0"),		/* DQ5 */
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "nand0"),		/* DQ4 */
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 12),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "nand0"),		/* DQS */
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 13),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "nand0"),		/* DQ3 */
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 14),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "nand0"),		/* DQ2 */
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 15),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "nand0"),		/* DQ1 */
		SUNXI_FUNCTION(0x4, "spi0"),		/* WP */
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 16),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "nand0"),		/* DQ0 */
		SUNXI_FUNCTION(0x4, "spi0"),		/* HOLD */
		SUNXI_FUNCTION(0x7, "io_disabled")),

	/* HOLE */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "i2s1"),		/* MCLK */
		SUNXI_FUNCTION(0x4, "uart2"),		/* RTS */
		SUNXI_FUNCTION(0x5, "vdevice"),		/* Vdevice*/
		SUNXI_FUNCTION(0x7, "io_disabled"),
		SUNXI_FUNCTION_IRQ_BANK(0x6, 1, 0)),	/* PE_EINT0 */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "spdif"),		/* OUT */
		SUNXI_FUNCTION(0x4, "spdif"),		/* IN */
		SUNXI_FUNCTION(0x5, "vdevice"),		/* Vdevice*/
		SUNXI_FUNCTION(0x7, "io_disabled"),
		SUNXI_FUNCTION_IRQ_BANK(0x6, 1, 1)),	/* PE_EINT1 */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "ledc"),		/* OUT */
		SUNXI_FUNCTION(0x7, "io_disabled"),
		SUNXI_FUNCTION_IRQ_BANK(0x6, 1, 2)),	/* PE_EINT2 */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "i2s1"),		/* LRCK */
		SUNXI_FUNCTION(0x4, "uart2"),		/* TX */
		SUNXI_FUNCTION(0x7, "io_disabled"),
		SUNXI_FUNCTION_IRQ_BANK(0x6, 1, 3)),	/* PE_EINT3 */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "i2s1"),		/* BCLK */
		SUNXI_FUNCTION(0x4, "uart2"),		/* RX */
		SUNXI_FUNCTION(0x7, "io_disabled"),
		SUNXI_FUNCTION_IRQ_BANK(0x6, 1, 4)),	/* PE_EINT4 */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "i2s1"),		/* DOUT0 */
		SUNXI_FUNCTION(0x3, "pll_lock_dbg"),
		SUNXI_FUNCTION(0x4, "i2s1_b"),		/* DIN1 */
		SUNXI_FUNCTION(0x7, "io_disabled"),
		SUNXI_FUNCTION_IRQ_BANK(0x6, 1, 5)),	/* PE_EINT5 */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "i2s1_b"),		/* DOUT1 */
		SUNXI_FUNCTION(0x3, "i2s1"),		/* DIN0 */
		SUNXI_FUNCTION(0x4, "uart2"),		/* CTS */
		SUNXI_FUNCTION(0x7, "io_disabled"),
		SUNXI_FUNCTION_IRQ_BANK(0x6, 1, 6)),	/* PE_EINT6 */
	/* HOLE */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "sdc1"),		/* CLK */
		SUNXI_FUNCTION(0x7, "io_disabled"),
		SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 0)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "sdc1"),		/* CMD */
		SUNXI_FUNCTION(0x7, "io_disabled"),
		SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 1)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "sdc1"),		/* D0 */
		SUNXI_FUNCTION(0x7, "io_disabled"),
		SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 2)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "sdc1"),		/* D1 */
		SUNXI_FUNCTION(0x7, "io_disabled"),
		SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 3)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "sdc1"),		/* D2 */
		SUNXI_FUNCTION(0x7, "io_disabled"),
		SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 4)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "sdc1"),		/* D3 */
		SUNXI_FUNCTION(0x7, "io_disabled"),
		SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 5)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart1"),		/* TX */
		SUNXI_FUNCTION(0x7, "io_disabled"),
		SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 6)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart1"),		/* RX */
		SUNXI_FUNCTION(0x7, "io_disabled"),
		SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 7)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart1"),		/* RTS */
		SUNXI_FUNCTION(0x7, "io_disabled"),
		SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 8)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart1"),		/* CTS */
		SUNXI_FUNCTION(0x7, "io_disabled"),
		SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 9)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x3, "i2s2"),		/* MCLK */
		SUNXI_FUNCTION(0x7, "io_disabled"),
		SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 10)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x3, "i2s2"),		/* LRCK */
		SUNXI_FUNCTION(0x5, "bist_result0"),
		SUNXI_FUNCTION(0x7, "io_disabled"),
		SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 11)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 12),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x3, "i2s2"),		/* BCLK */
		SUNXI_FUNCTION(0x5, "bist_result1"),
		SUNXI_FUNCTION(0x7, "io_disabled"),
		SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 12)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 13),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x3, "i2s2"),		/* DOUT0 */
		SUNXI_FUNCTION(0x4, "i2s2_b"),		/* DIN1 */
		SUNXI_FUNCTION(0x7, "io_disabled"),
		SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 13)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 14),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x3, "i2s2_b"),		/* DOUT1 */
		SUNXI_FUNCTION(0x4, "i2s2"),		/* DIN0 */
		SUNXI_FUNCTION(0x7, "io_disabled"),
		SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 14)),

	/* HOLE */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "twi0"),		/* SCK */
		SUNXI_FUNCTION(0x3, "uart0"),		/* TX */
		SUNXI_FUNCTION(0x4, "spi1"),		/* CS */
		SUNXI_FUNCTION(0x7, "io_disabled"),
		SUNXI_FUNCTION_IRQ_BANK(0x6, 3, 0)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "twi0"),		/* SDA */
		SUNXI_FUNCTION(0x3, "uart0"),		/* RX */
		SUNXI_FUNCTION(0x4, "spi1"),		/* CLK */
		SUNXI_FUNCTION(0x7, "io_disabled"),
		SUNXI_FUNCTION_IRQ_BANK(0x6, 3, 1)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "twi1"),		/* SCK */
		SUNXI_FUNCTION(0x3, "ledc"),		/* DO */
		SUNXI_FUNCTION(0x4, "spi1"),		/* MOSI */
		SUNXI_FUNCTION(0x7, "io_disabled"),
		SUNXI_FUNCTION_IRQ_BANK(0x6, 3, 2)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "twi1"),		/* SDA */
		SUNXI_FUNCTION(0x3, "spdif"),		/* OUT */
		SUNXI_FUNCTION(0x4, "spi1"),		/* MISO */
		SUNXI_FUNCTION(0x7, "io_disabled"),
		SUNXI_FUNCTION_IRQ_BANK(0x6, 3, 3)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart3"),		/* TX */
		SUNXI_FUNCTION(0x3, "spi1"),		/* CS */
		SUNXI_FUNCTION(0x7, "io_disabled"),
		SUNXI_FUNCTION_IRQ_BANK(0x6, 3, 4)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart3"),		/* RX */
		SUNXI_FUNCTION(0x3, "spi1"),		/* CLK */
		SUNXI_FUNCTION(0x7, "io_disabled"),
		SUNXI_FUNCTION_IRQ_BANK(0x6, 3, 5)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart3"),		/* RTS */
		SUNXI_FUNCTION(0x3, "spi1"),		/* MOSI */
		SUNXI_FUNCTION(0x7, "io_disabled"),
		SUNXI_FUNCTION_IRQ_BANK(0x6, 3, 6)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart3"),		/* CTS */
		SUNXI_FUNCTION(0x3, "spi1"),		/* MISO */
		SUNXI_FUNCTION(0x7, "io_disabled"),
		SUNXI_FUNCTION_IRQ_BANK(0x6, 3, 7)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "ledc"),		/* DO */
		SUNXI_FUNCTION(0x3, "spdif"),		/* IN */
		SUNXI_FUNCTION(0x7, "io_disabled"),
		SUNXI_FUNCTION_IRQ_BANK(0x6, 3, 8)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x4, "cpu_cur_w"),
		SUNXI_FUNCTION(0x7, "io_disabled"),
		SUNXI_FUNCTION_IRQ_BANK(0x6, 3, 9)),
};

static const unsigned int sun8iw18p1_irq_bank_map[] = {
	SUNXI_BANK_OFFSET('B', 'A'),
	SUNXI_BANK_OFFSET('E', 'A'),
	SUNXI_BANK_OFFSET('G', 'A'),
	SUNXI_BANK_OFFSET('H', 'A'),
};
static const struct sunxi_pinctrl_desc sun8iw18p1_pinctrl_data = {
	.pins = sun8iw18p1_pins,
	.npins = ARRAY_SIZE(sun8iw18p1_pins),
	.irq_banks = ARRAY_SIZE(sun8iw18p1_irq_bank_map),
	.irq_bank_map = sun8iw18p1_irq_bank_map,
	.pin_base = 0,
	.io_bias_cfg_variant = BIAS_VOLTAGE_PIO_POW_MODE_CTL,
	.pf_power_source_switch = true,
	.hw_type = SUNXI_PCTL_HW_TYPE_0,
};

static void *mem;
static int mem_size;

static int sun8iw18p1_pinctrl_probe(struct platform_device *pdev)
{
	struct resource *res;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -EINVAL;
	mem_size = resource_size(res);

	mem = devm_kzalloc(&pdev->dev, mem_size, GFP_KERNEL);
	if (!mem)
		return -ENOMEM;

	return sunxi_bsp_pinctrl_init(pdev, &sun8iw18p1_pinctrl_data);
}

static int __maybe_unused sun8iw18p1_pinctrl_suspend_noirq(struct device *dev)
{
	struct sunxi_pinctrl *pctl = dev_get_drvdata(dev);
	unsigned long flags;

	raw_spin_lock_irqsave(&pctl->lock, flags);
	memcpy(mem, pctl->membase, mem_size);
	raw_spin_unlock_irqrestore(&pctl->lock, flags);

	return 0;
}

static int __maybe_unused sun8iw18p1_pinctrl_resume_noirq(struct device *dev)
{
	struct sunxi_pinctrl *pctl = dev_get_drvdata(dev);
	unsigned long flags;

	raw_spin_lock_irqsave(&pctl->lock, flags);
	memcpy(pctl->membase, mem, mem_size);
	raw_spin_unlock_irqrestore(&pctl->lock, flags);

	return 0;
}

static const struct of_device_id sun8iw18p1_pinctrl_match[] = {
	{ .compatible = "allwinner,sun8iw18p1-pinctrl", },
	{}
};
MODULE_DEVICE_TABLE(of, sun8iw18p1_pinctrl_match);

static const struct dev_pm_ops sun8iw18p1_pinctrl_pm_ops = {
	.suspend_noirq = sun8iw18p1_pinctrl_suspend_noirq,
	.resume_noirq = sun8iw18p1_pinctrl_resume_noirq,
};

static struct platform_driver sun8iw18p1_pinctrl_driver = {
	.probe	= sun8iw18p1_pinctrl_probe,
	.driver	= {
		.name		= "sun8iw18p1-pinctrl",
		.owner		= THIS_MODULE,
		.of_match_table	= sun8iw18p1_pinctrl_match,
		.pm		= &sun8iw18p1_pinctrl_pm_ops,
	},
};

static int __init sun8iw18p1_pio_init(void)
{
	int ret;

	ret = platform_driver_register(&sun8iw18p1_pinctrl_driver);
	if (ret) {
		pr_err("register sun8iw18p1 pio controller failed\n");
		return -EINVAL;
	}
	return 0;
}
postcore_initcall(sun8iw18p1_pio_init);

MODULE_AUTHOR("huangshuosheng<huangshuosheng@allwinnertech.com>");
MODULE_DESCRIPTION("Allwinner sun8iw18p1 pio pinctrl driver");
MODULE_LICENSE("GPL");
