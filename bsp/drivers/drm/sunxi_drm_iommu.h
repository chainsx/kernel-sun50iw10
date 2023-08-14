/* sunxi_drm_iommu.h
 *
 * Copyright (C) 2022 Allwinnertech Co., Ltd.
 * Authors: hongyaobin <hongyaobin@allwinnertech.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#ifndef _SUNXI_DRM_IOMMU_H_
#define _SUNXI_DRM_IOMMU_H_

#ifdef CONFIG_AW_DRM_IOMMU

#if defined(CONFIG_ARM_DMA_USE_IOMMU)
#include <asm/dma-iommu.h>

static inline int __sunxi_iommu_create_mapping(struct sunxi_drm_private *priv,
					unsigned long start, unsigned long size)
{
	DRM_INFO("[DRM-DRV] %s:arm_iommu_create_mapping\n", __func__);
	priv->mapping = arm_iommu_create_mapping(&platform_bus_type, start,
						 size);
	return IS_ERR(priv->mapping);
}

static inline void
__sunxi_iommu_release_mapping(struct sunxi_drm_private *priv)
{
	arm_iommu_release_mapping(priv->mapping);
}

static inline int __sunxi_iommu_attach(struct sunxi_drm_private *priv,
					struct device *dev)
{
	if (dev->archdata.mapping)
		arm_iommu_detach_device(dev);

	return arm_iommu_attach_device(dev, priv->mapping);
}

static inline void __sunxi_iommu_detach(struct sunxi_drm_private *priv,
					 struct device *dev)
{
	arm_iommu_detach_device(dev);
}

#elif defined(CONFIG_IOMMU_DMA)
#include <linux/dma-iommu.h>

static inline int __sunxi_iommu_create_mapping(struct sunxi_drm_private *priv,
					unsigned long start, unsigned long size)
{
	DRM_INFO("[DRM-DRV] %s\n", __func__);
	priv->mapping = iommu_get_domain_for_dev(priv->drm_dev->dev);
	if (priv->mapping == NULL)
		return -1;
	else
		return 0;
}

static inline void __sunxi_iommu_release_mapping(struct sunxi_drm_private *priv)
{
	priv->mapping = NULL;
}

static inline int __sunxi_iommu_attach(struct sunxi_drm_private *priv,
					struct device *dev)
{
	struct iommu_domain *domain = priv->mapping;

	if (dev != priv->drm_dev->dev)
		return iommu_attach_device(domain, dev);
	return 0;
}

static inline void __sunxi_iommu_detach(struct sunxi_drm_private *priv,
					 struct device *dev)
{
	struct iommu_domain *domain = priv->mapping;

	if (dev != priv->drm_dev->dev)
		iommu_detach_device(domain, dev);
}
#else
#error Unsupported architecture and IOMMU/DMA-mapping glue code
#endif

int sunxi_drm_create_iommu_mapping(struct drm_device *drm_dev);

void sunxi_drm_release_iommu_mapping(struct drm_device *drm_dev);

int sunxi_drm_iommu_attach_device(struct drm_device *drm_dev,
				struct device *subdrv_dev);

void sunxi_drm_iommu_detach_device(struct drm_device *dev_dev,
				struct device *subdrv_dev);

static inline bool is_drm_iommu_supported(struct drm_device *drm_dev)
{
	struct sunxi_drm_private *priv = drm_dev->dev_private;

	return priv->mapping ? true : false;
}

#else

static inline int sunxi_drm_create_iommu_mapping(struct drm_device *drm_dev)
{
	return 0;
}

static inline void sunxi_drm_release_iommu_mapping(struct drm_device *drm_dev)
{
}

static inline int sunxi_drm_iommu_attach_device(struct drm_device *drm_dev,
						struct device *subdrv_dev)
{
	return 0;
}

static inline void sunxi_drm_iommu_detach_device(struct drm_device *drm_dev,
						struct device *subdrv_dev)
{
}

static inline bool is_drm_iommu_supported(struct drm_device *drm_dev)
{
	return false;
}

#endif
#endif
