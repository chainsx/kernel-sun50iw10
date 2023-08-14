/*
 *
 * Copyright (C) 2015 AllWinnertech Ltd.
 *
 * Author: huangshuosheng <huangshuosheng@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __LINUX_SUNXI_IOMMU_H
#define __LINUX_SUNXI_IOMMU_H

typedef void (*sunxi_iommu_fault_cb)(void);
extern void sunxi_iommu_register_fault_cb(sunxi_iommu_fault_cb cb, unsigned int master_id);
extern void sunxi_enable_device_iommu(unsigned int master_id, bool flag);
extern void sunxi_reset_device_iommu(unsigned int master_id);

#endif  /* __LINUX_SUNXI_IOMMU_H */
