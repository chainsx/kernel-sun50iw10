/*
 * linux-5.4/drivers/media/platform/sunxi-vin/utility/vin_os.c
 *
 * Copyright (c) 2007-2017 Allwinnertech Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/module.h>
#include "vin_os.h"
#include <linux/dma-heap.h>
#include <linux/dma-buf.h>

unsigned int vin_log_mask;
EXPORT_SYMBOL_GPL(vin_log_mask);

int os_gpio_write(u32 gpio, __u32 out_value, int force_value_flag)
{
#ifndef FPGA_VER
	if (gpio == GPIO_INDEX_INVALID)
		return 0;

	if (force_value_flag == 1) {
		gpio_direction_output(gpio, 0);
		__gpio_set_value(gpio, out_value);
	} else {
		if (out_value == 0) {
			gpio_direction_output(gpio, 0);
			__gpio_set_value(gpio, out_value);
		} else {
			gpio_direction_input(gpio);
		}
	}
#endif
	return 0;
}
EXPORT_SYMBOL_GPL(os_gpio_write);

int vin_get_ion_phys(struct device *dev, struct vin_mm *mem_man)
{
	struct dma_buf_attachment *attachment;
	struct sg_table *sgt;
	int ret = -1;

	if (IS_ERR(mem_man->buf)) {
		pr_err("dma_buf is null\n");
		return ret;
	}

	attachment = dma_buf_attach(mem_man->buf, get_device(dev));
	if (IS_ERR(attachment)) {
		pr_err("dma_buf_attach failed\n");
		goto err_buf_put;
	}

	sgt = dma_buf_map_attachment(attachment, DMA_FROM_DEVICE);
	if (IS_ERR_OR_NULL(sgt)) {
		pr_err("dma_buf_map_attachment failed\n");
		goto err_buf_detach;
	}

	mem_man->phy_addr = (void *)sg_dma_address(sgt->sgl);
	mem_man->sgt = sgt;
	mem_man->attachment = attachment;
	ret = 0;
	goto exit;

err_buf_detach:
	dma_buf_detach(mem_man->buf, attachment);
err_buf_put:
	dma_buf_put(mem_man->buf);
exit:
	return ret;

}

void vin_free_ion_phys(struct device *dev, struct vin_mm *mem_man)
{
	dma_buf_unmap_attachment(mem_man->attachment, mem_man->sgt, DMA_FROM_DEVICE);
	dma_buf_detach(mem_man->buf, mem_man->attachment);
	dma_buf_put(mem_man->buf);

}
int os_mem_alloc(struct device *dev, struct vin_mm *mem_man)
{
	int ret = -1;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
	struct dma_buf_map map;
#endif

	if (mem_man == NULL)
		return -1;

#ifdef SUNXI_MEM
#if IS_ENABLED(CONFIG_AW_IOMMU) && IS_ENABLED(CONFIG_VIN_IOMMU)
	/* DMA BUFFER HEAP (after linux 5.10)*/
	mem_man->dmaHeap = dma_heap_find("system-uncached");
	mem_man->buf = dma_heap_buffer_alloc(mem_man->dmaHeap, mem_man->size, O_RDWR, 0);
	if (IS_ERR(mem_man->buf)) {
		vin_err("dma_heap_buffer_alloc failed\n");
		goto err_alloc;
	}
#else
	/* CMA or CARVEOUT */
	mem_man->dmaHeap = dma_heap_find("reserved");
	mem_man->buf = dma_heap_buffer_alloc(mem_man->dmaHeap, mem_man->size, O_RDWR, 0);
	if (IS_ERR(mem_man->buf)) {
		vin_err("dma_heap_buffer alloc failed\n");
		goto err_alloc;
	}
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
	ret = dma_buf_vmap(mem_man->buf, &map);
	if (ret) {
		vin_err("dma_buf_vmap failed!!");
		goto err_map_kernel;
	}
	mem_man->vir_addr = map.vaddr;
#else /* before linux-5.15 */
	mem_man->vir_addr = dma_buf_vmap(mem_man->buf);
	if (IS_ERR_OR_NULL(mem_man->vir_addr)) {
		vin_err("ion_map_kernel failed!!");
		goto err_map_kernel;
	}
#endif

	/*IOMMU or CMA or CARVEOUT */
	ret = vin_get_ion_phys(dev, mem_man);
	if (ret) {
		vin_err("ion_phys failed!!");
		goto err_phys;
	}
	mem_man->dma_addr = mem_man->phy_addr;
	return ret;

err_phys:

	dma_buf_vunmap(mem_man->buf, mem_man->vir_addr);

err_map_kernel:

	dma_heap_buffer_free(mem_man->buf);


err_alloc:
	return ret;
#else
	mem_man->vir_addr = dma_alloc_coherent(dev, (size_t) mem_man->size,
					(dma_addr_t *)&mem_man->phy_addr,
					GFP_KERNEL);
	if (!mem_man->vir_addr) {
		vin_err("dma_alloc_coherent memory alloc failed\n");
		return -ENOMEM;
	}
	mem_man->dma_addr = mem_man->phy_addr;
	ret = 0;
	return ret;
#endif
}
EXPORT_SYMBOL_GPL(os_mem_alloc);


void os_mem_free(struct device *dev, struct vin_mm *mem_man)
{

	if (mem_man == NULL)
		return;

#ifdef SUNXI_MEM
	vin_free_ion_phys(dev, mem_man);
	//ion_heap_unmap_kernel(mem_man->heap, mem_man->buf->priv);
	dma_buf_vunmap(mem_man->buf, mem_man->vir_addr);
	//ion_free(mem_man->buf->priv);
	dma_heap_buffer_free(mem_man->buf);
#else
	if (mem_man->vir_addr)
		dma_free_coherent(dev, mem_man->size, mem_man->vir_addr,
				  (dma_addr_t) mem_man->phy_addr);
#endif
	mem_man->phy_addr = NULL;
	mem_man->dma_addr = NULL;
	mem_man->vir_addr = NULL;
}
EXPORT_SYMBOL_GPL(os_mem_free);

MODULE_AUTHOR("raymonxiu");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("Video front end OSAL for sunxi");
