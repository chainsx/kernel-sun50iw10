/* sound\soc\sunxi\snd_sunxi_mach.h
 * (C) Copyright 2021-2025
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Dby <dby@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef __SND_SUNXI_MACH_H
#define __SND_SUNXI_MACH_H

#include "snd_sunxi_mach_utils.h"

#if IS_ENABLED(CONFIG_SND_SOC_SUNXI_DEBUG)
static inline int snd_sunxi_sysfs_create_group(struct platform_device *pdev,
					       struct attribute_group *attr)
{
	return sysfs_create_group(&pdev->dev.kobj, attr);
}

static inline void snd_sunxi_sysfs_remove_group(struct platform_device *pdev,
						struct attribute_group *attr)
{
	sysfs_remove_group(&pdev->dev.kobj, attr);
}
#else
static inline int snd_sunxi_sysfs_create_group(struct platform_device *pdev,
					       struct attribute_group *attr)
{
	return 0;
}

static inline void snd_sunxi_sysfs_remove_group(struct platform_device *pdev,
						struct attribute_group *attr)
{
}
#endif

#endif /* __SND_SUNXI_MACH_H */
