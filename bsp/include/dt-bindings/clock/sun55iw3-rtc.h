// SPDX-License-Identifier: (GPL-2.0+ or MIT)
/*
 * Copyright (C) 2022 rengaomin@allwinnertech.com
 */

#ifndef _DT_BINDINGS_CLK_SUN55IW3_RTC_H_
#define _DT_BINDINGS_CLK_SUN55IW3_RTC_H_

#define CLK_IOSC		0
#define CLK_EXT32K_GATE		1
#define CLK_IOSC_DIV32K		2
#define CLK_OSC32K		3
#define CLK_DCXO24M_DIV32K	4
#define CLK_RTC32K		5
#define CLK_RTC_1K		6
#define CLK_RTC_32K_FANOUT	7
#define CLK_RTC_SPI		8

#define CLK_RTC_MAX_NO		CLK_RTC_SPI

#endif /* _DT_BINDINGS_CLK_SUN55IW3_RTC_H_ */
