/******************************************************************************
 *
 * Copyright(c) 2013 - 2017 Realtek Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 *****************************************************************************/
#include <drv_types.h>
#ifdef CONFIG_GPIO_WAKEUP
#include <linux/gpio.h>
#endif

#ifdef CONFIG_MMC
extern void sunxi_mmc_rescan_card(unsigned ids);
extern void sunxi_wlan_set_power(int on);
extern int sunxi_wlan_get_bus_index(void);
extern int sunxi_wlan_get_oob_irq(void);
extern int sunxi_wlan_get_oob_irq_flags(void);
#ifdef CONFIG_GPIO_WAKEUP
extern unsigned int oob_irq;
#endif
#endif /* CONFIG_MMC */

void platform_wifi_mac_addr(u8 *mac_addr)
{
}

/*
 * Return:
 *	0:	power on successfully
 *	others: power on failed
 */
int platform_wifi_power_on(void)
{
	int ret = 0;
#ifdef CONFIG_MMC
	int wlan_bus_index = sunxi_wlan_get_bus_index();
	if (wlan_bus_index < 0)
		return wlan_bus_index;

	sunxi_wlan_set_power(1);
	mdelay(100);
	sunxi_mmc_rescan_card(wlan_bus_index);
	RTW_INFO("%s: power up, rescan card.\n", __func__);

#ifdef CONFIG_GPIO_WAKEUP
	oob_irq = sunxi_wlan_get_oob_irq();
#endif /* CONFIG_GPIO_WAKEUP */
#endif /* CONFIG_MMC */

	return ret;
}

void platform_wifi_power_off(void)
{
#ifdef CONFIG_MMC
	int wlan_bus_index = sunxi_wlan_get_bus_index();
	if (wlan_bus_index < 0)
		return;

	sunxi_wlan_set_power(0);
	mdelay(100);
	sunxi_mmc_rescan_card(wlan_bus_index);
	RTW_INFO("%s: remove card, power off.\n", __func__);
#endif /* CONFIG_MMC */
}
