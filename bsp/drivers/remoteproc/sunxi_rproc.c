/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright © 2020-2025, Allwinnertech
 *
 * This file is provided under a dual BSD/GPL license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

/* #define DEBUG */
#include <linux/arm-smccc.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/remoteproc.h>
#include <linux/io.h>
#include <linux/mailbox_client.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_reserved_mem.h>
#include <linux/pm_wakeirq.h>
#include <linux/regmap.h>
#include <linux/remoteproc.h>
#include <linux/slab.h>
#include <linux/pinctrl/consumer.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/clk.h>
#include "remoteproc_internal.h"
#include "sunxi_rproc_boot.h"

#define SUNXI_RPROC_VERSION "2.1.0"

#define MBOX_NB_VQ		2

static LIST_HEAD(sunxi_rproc_list);
struct sunxi_mbox {
	struct mbox_chan *chan;
	struct mbox_client client;
	struct work_struct vq_work;
	int vq_id;
};

struct sunxi_rproc {
	struct sunxi_rproc_priv *rproc_priv;  /* dsp/riscv private resources */
	struct sunxi_mbox mb;
	struct workqueue_struct *workqueue;
	struct list_head list;
	struct rproc *rproc;
	char *name;
};

extern int simulator_debug;

static int sunxi_rproc_pa_to_da(struct rproc *rproc, phys_addr_t pa, u64 *da)
{
	struct device *dev = rproc->dev.parent;
	struct sunxi_rproc *chip = rproc->priv;
	struct sunxi_rproc_memory_mapping *map;
	int i;

	/*
	 * Maybe there are multiple DAs corresponding to one PA.
	 * Here we only return the first matching one in the map table.
	 */
	for (i = 0; i < chip->rproc_priv->mem_maps_cnt; i++) {
		map = &chip->rproc_priv->mem_maps[i];
		if (pa < map->pa || pa >= map->pa + map->len)
			continue;
		*da = pa - map->pa + map->da;
		dev_dbg(dev, "translate pa %pa to da 0x%llx\n", &pa, *da);
		return 0;
	}

	dev_err(dev, "Failed to translate pa %pa to da\n", &pa);
	return -EINVAL;
}

static int sunxi_rproc_da_to_pa(struct rproc *rproc, u64 da, phys_addr_t *pa)
{
	struct device *dev = rproc->dev.parent;
	struct sunxi_rproc *chip = rproc->priv;
	struct sunxi_rproc_memory_mapping *map;
	int i;

	for (i = 0; i < chip->rproc_priv->mem_maps_cnt; i++) {
		map = &chip->rproc_priv->mem_maps[i];
		if (da < map->da || da >= map->da + map->len)
			continue;
		*pa = da - map->da + map->pa;
		dev_dbg(dev, "translate da 0x%llx to pa %pa\n", da, pa);
		return 0;
	}

	dev_err(dev, "Failed to translate da 0x%llx to pa\n", da);
	return -EINVAL;
}

static int sunxi_rproc_mem_alloc(struct rproc *rproc,
				 struct rproc_mem_entry *mem)
{
	struct device *dev = rproc->dev.parent;
	void *va;

	dev_dbg(dev, "map memory: %pad+%lx\n", &mem->dma, mem->len);
	va = ioremap_wc(mem->dma, mem->len);
	if (IS_ERR_OR_NULL(va)) {
		dev_err(dev, "Unable to map memory region: %pad+%lx\n",
			&mem->dma, mem->len);
		return -ENOMEM;
	}

	/* Update memory entry va */
	mem->va = va;

	return 0;
}

static int sunxi_rproc_mem_release(struct rproc *rproc,
				   struct rproc_mem_entry *mem)
{
	dev_dbg(rproc->dev.parent, "unmap memory: %pad\n", &mem->dma);
	iounmap(mem->va);

	return 0;
}

static void *sunxi_rproc_da_to_va(struct rproc *rproc, u64 da, size_t len, bool *is_iomem)
{
	struct device *dev = rproc->dev.parent;
	struct rproc_mem_entry *carveout;
	void *ptr = NULL;
	phys_addr_t pa;
	int ret;

	dev_dbg(dev, "%s,%d\n", __func__, __LINE__);

	/* first step: translate da to pa */
	ret = sunxi_rproc_da_to_pa(rproc, da, &pa);
	if (ret) {
		dev_err(dev, "invalid da 0x%llx\n", da);
		return NULL;
	}

	/* second step: get va from carveouts via pa */
	list_for_each_entry(carveout, &rproc->carveouts, node) {
		if ((pa >= carveout->dma) && (pa < carveout->dma + carveout->len)) {
			ptr = carveout->va + (pa - carveout->dma);
			return ptr;
		}
	}
	return NULL;
}

static void sunxi_rproc_mb_vq_work(struct work_struct *work)
{
	struct sunxi_mbox *mb = container_of(work, struct sunxi_mbox, vq_work);
	struct rproc *rproc = dev_get_drvdata(mb->client.dev);

	dev_dbg(&rproc->dev, "%s,%d\n", __func__, __LINE__);

	/*
	 * We put the data receiving and processing part
	 * of the virtqueue in the bottom half.
	 */
	if (rproc_vq_interrupt(rproc, mb->vq_id) == IRQ_NONE)
		dev_dbg(&rproc->dev, "no message found in vq%d\n", mb->vq_id);
}

static void sunxi_rproc_mb_rx_callback(struct mbox_client *cl, void *data)
{
	struct rproc *rproc = dev_get_drvdata(cl->dev);
	struct sunxi_mbox *mb = container_of(cl, struct sunxi_mbox, client);
	struct sunxi_rproc *chip = rproc->priv;

	dev_dbg(&rproc->dev, "%s,%d name = arm-kick, vq_id = 0x%x\n", __func__, __LINE__, mb->vq_id);

	/*
	 * Data is sent from remote processor,
	 * which represents the virtqueue ID.
	 */
	mb->vq_id = *(u32 *)data;

	queue_work(chip->workqueue, &mb->vq_work);
}

static void sunxi_rproc_mb_tx_done(struct mbox_client *cl, void *msg, int r)
{
	struct rproc *rproc = dev_get_drvdata(cl->dev);
	struct sunxi_mbox *mb = container_of(cl, struct sunxi_mbox, client);

	dev_dbg(&rproc->dev, "%s,%d name = arm-kick, vq_id = 0x%x\n", __func__, __LINE__, mb->vq_id);
	devm_kfree(&rproc->dev, msg);
}

static int sunxi_rproc_request_mbox(struct rproc *rproc)
{
	struct sunxi_rproc *chip = rproc->priv;
	struct device *dev = &rproc->dev;

	/* Initialise mailbox structure table */
	chip->mb.client.rx_callback = sunxi_rproc_mb_rx_callback;
	chip->mb.client.tx_done = sunxi_rproc_mb_tx_done;
	chip->mb.client.tx_block = false;
	chip->mb.client.dev = dev->parent;

	chip->mb.chan = mbox_request_channel_byname(&chip->mb.client, "arm-kick");
	if (IS_ERR(chip->mb.chan)) {
		if (PTR_ERR(chip->mb.chan) == -EPROBE_DEFER)
			goto err_probe;
		dev_warn(dev, "cannot get arm-kick mbox\n");
		chip->mb.chan = NULL;
	}

	INIT_WORK(&chip->mb.vq_work, sunxi_rproc_mb_vq_work);

	return 0;

err_probe:
	mbox_free_channel(chip->mb.chan);
	return -EPROBE_DEFER;
}

static void sunxi_rproc_free_mbox(struct rproc *rproc)
{
	struct sunxi_rproc *chip = rproc->priv;

	mbox_free_channel(chip->mb.chan);
	chip->mb.chan = NULL;
}

static int sunxi_rproc_start(struct rproc *rproc)
{
	struct sunxi_rproc *chip = rproc->priv;
	struct sunxi_rproc_priv *rproc_priv = chip->rproc_priv;
	int ret;

	ret = sunxi_rproc_priv_start(rproc_priv);
	if (ret) {
		dev_err(rproc_priv->dev, "start remoteproc error\n");
		return ret;
	}

	return 0;
}

static int sunxi_rproc_stop(struct rproc *rproc)
{
	struct sunxi_rproc *chip = rproc->priv;
	struct sunxi_rproc_priv *rproc_priv = chip->rproc_priv;
	int ret;

	ret = sunxi_rproc_priv_stop(rproc_priv);
	if (ret) {
		dev_err(rproc_priv->dev, "stop remoteproc error\n");
		return ret;
	}

	return 0;
}

static int sunxi_rproc_parse_fw(struct rproc *rproc, const struct firmware *fw)
{
	int ret = 0;
	struct sunxi_rproc *chip = rproc->priv;
	struct elf32_hdr *ehdr  = (struct elf32_hdr *)fw->data;
	struct device *dev = rproc->dev.parent;
	struct device_node *np = dev->of_node;
	struct of_phandle_iterator it;
	struct rproc_mem_entry *mem;
	struct reserved_mem *rmem;
	int index = 0;
	u64 da;

	dev_dbg(dev, "%s,%d\n", __func__, __LINE__);

	ret = of_phandle_iterator_init(&it, np, "memory-region", NULL, 0);
	if (ret) {
		dev_err(dev, "memory-region iterator init fail %d\n", ret);
		return -ENODEV;
	}

	while (of_phandle_iterator_next(&it) == 0) {
		rmem = of_reserved_mem_lookup(it.node);
		if (!rmem) {
			dev_err(dev, "unable to acquire memory-region\n");
			return -EINVAL;
		}

		ret = sunxi_rproc_pa_to_da(rproc, rmem->base, &da);
		if (ret) {
			dev_err(dev, "memory region not valid: %pa\n", &rmem->base);
			return -EINVAL;
		}

		/* No need to map vdev buffer */
		if (0 == strcmp(it.node->name, "vdev0buffer")) {
			mem = rproc_of_resm_mem_entry_init(dev, index,
							   rmem->size,
							   da,
							   it.node->name);
			/*
			 * The rproc_of_resm_mem_entry_init didn't save the
			 * physical address. Here we save it manually.
			 */
			if (mem)
				mem->dma = (dma_addr_t)rmem->base;
		} else {
			mem = rproc_mem_entry_init(dev, NULL,
						   (dma_addr_t)rmem->base,
						   rmem->size, da,
						   sunxi_rproc_mem_alloc,
						   sunxi_rproc_mem_release,
						   it.node->name);
			if (mem)
				rproc_coredump_add_segment(rproc, da,
							   rmem->size);
		}

		if (!mem)
			return -ENOMEM;

		rproc_add_carveout(rproc, mem);
		index++;
	}

	chip->rproc_priv->pc_entry = ehdr->e_entry;

	/* check segment name, such as .resource_table */
	ret = rproc_elf_load_rsc_table(rproc, fw);
	if (ret != 0) {
		rproc->cached_table = NULL;
		rproc->table_ptr = NULL;
		rproc->table_sz = 0;
		dev_warn(&rproc->dev, "no resource table found for this firmware\n");
	}

	return ret;
}

static void sunxi_rproc_kick(struct rproc *rproc, int vqid)
{
	struct sunxi_rproc *chip = rproc->priv;
	u32 *msg = NULL;
	int err;

	dev_dbg(&rproc->dev, "%s,%d vqid = 0x%x\n", __func__, __LINE__, vqid);

	if (WARN_ON(vqid >= MBOX_NB_VQ))
		return;

	/*
	 * Because of the implementation of sunxi msgbox(mailbox controller),
	 * the type of mailbox message should be u32.
	 */
	msg = devm_kzalloc(&rproc->dev, sizeof(*msg), GFP_KERNEL);
	if (!msg)
		return;

	*msg = vqid;

	/* Remeber to free msg in mailbox tx_done callback */
	err = mbox_send_message(chip->mb.chan, (void *)msg);
	if (err < 0)
		dev_err(&rproc->dev, "%s,%d kick err:%d\n",
			__func__, __LINE__, err);
	return;
}

static int sunxi_rproc_elf_find_segments(struct rproc *rproc,
					 const struct firmware *fw,
					 const char *find_name,
					 struct elf32_shdr **find_shdr,
					 struct elf32_phdr **find_phdr)
{
	struct device *dev = &rproc->dev;
	struct elf32_hdr *ehdr;
	struct elf32_shdr *shdr;
	struct elf32_phdr *phdr;
	const char *name_table;
	const u8 *elf_data = fw->data;
	u32 i, j, size;

	ehdr = (struct elf32_hdr *)elf_data;
	shdr = (struct elf32_shdr *)(elf_data + ehdr->e_shoff);
	phdr = (struct elf32_phdr *)(elf_data + ehdr->e_phoff);

	name_table = elf_data + shdr[ehdr->e_shstrndx].sh_offset;
	for (i = 0; i < ehdr->e_shnum; i++, shdr++) {
		size = shdr->sh_size;

		if (strcmp(name_table + shdr->sh_name, find_name))
			continue;

		*find_shdr = shdr;
		dev_dbg(dev, "%s,%d %s addr 0x%x, size 0x%x\n",
			__func__, __LINE__, find_name, shdr->sh_addr, size);

		for (j = 0; j < ehdr->e_phnum; j++, phdr++) {
			if (shdr->sh_addr == phdr->p_paddr) {
				*find_phdr = phdr;
				dev_dbg(dev, "%s,%d find %s phdr\n",
					__func__, __LINE__, find_name);
				return 0;
			}
		}
	}

	return -EINVAL;

}

static int sunxi_rproc_elf_load_segments(struct rproc *rproc, const struct firmware *fw)
{
	struct device *dev = &rproc->dev;
	struct sunxi_rproc *chip = rproc->priv;
	struct elf32_hdr *ehdr;
	struct elf32_phdr *phdr;
	struct elf32_shdr *shdr;
	int i, ret = 0;
	const u8 *elf_data = fw->data;
	u32 offset, da, memsz, filesz;
	void *ptr;

	/* get version from elf  */
	ret = sunxi_rproc_elf_find_segments(rproc, fw, ".version_table", &shdr, &phdr);
	if (ret) {
		dev_warn(dev, "%s,%d find segments err\n", __func__, __LINE__);
		/* Lack of ".version_table" should not be assumed as an error */
		ret = 0;
	} else {
		dev_info(dev, "dsp elf version: %s\n", elf_data + phdr->p_offset);
	}

	/* we must copy .resource_table, when use simulator to debug */
	if (simulator_debug) {
		dev_dbg(dev, "%s,%d only load .resource_table data\n",
				__func__, __LINE__);

		ret = sunxi_rproc_elf_find_segments(rproc, fw, ".resource_table", &shdr, &phdr);
		if (ret) {
			dev_err(dev, "%s,%d find segments err\n", __func__, __LINE__);
			return ret;
		}

		da = phdr->p_paddr;
		memsz = phdr->p_memsz;
		filesz = phdr->p_filesz;
		ptr = rproc_da_to_va(rproc, da, memsz, NULL);
		if (!ptr) {
			dev_err(dev, "bad phdr da 0x%x mem 0x%x\n", da, memsz);
			return -EINVAL;
		}
		if (phdr->p_filesz)
			memcpy(ptr, elf_data + phdr->p_offset, filesz);

		if (memsz > filesz)
			memset(ptr + filesz, 0, memsz - filesz);

		return 0;
	}

	/* get ehdr & phdr addr */
	ehdr = (struct elf32_hdr *)elf_data;
	phdr = (struct elf32_phdr *)(elf_data + ehdr->e_phoff);

	/* arm can write/read local ram */
	sunxi_rproc_priv_set_localram(chip->rproc_priv, 1);

	dev_dbg(dev, "%s,%d\n", __func__, __LINE__);

	/* go through the available ELF segments */
	for (i = 0; i < ehdr->e_phnum; i++, phdr++) {
		da = phdr->p_paddr;
		memsz = phdr->p_memsz;
		filesz = phdr->p_filesz;
		offset = phdr->p_offset;

		if (phdr->p_type != PT_LOAD)
			continue;

		dev_dbg(dev, "phdr: type %d da 0x%x memsz 0x%x filesz 0x%x\n",
			phdr->p_type, da, memsz, filesz);

		if ((memsz == 0) || (filesz == 0))
			continue;

		if (filesz > memsz) {
			dev_err(dev, "bad phdr filesz 0x%x memsz 0x%x\n",
				filesz, memsz);
			ret = -EINVAL;
			break;
		}

		if (offset + filesz > fw->size) {
			dev_err(dev, "truncated fw: need 0x%x avail 0x%zx\n",
				offset + filesz, fw->size);
			ret = -EINVAL;
			break;
		}

		/* grab the kernel address for this device address */
		ptr = rproc_da_to_va(rproc, da, memsz, NULL);
		if (!ptr) {
			dev_err(dev, "bad phdr da 0x%x mem 0x%x\n", da, memsz);
			ret = -EINVAL;
			break;
		}

		/* put the segment where the remote processor expects it */
		if (phdr->p_filesz)
			memcpy(ptr, elf_data + phdr->p_offset, filesz);

		/*
		 * Zero out remaining memory for this segment.
		 *
		 * This isn't strictly required since dma_alloc_coherent already
		 * did this for us. albeit harmless, we may consider removing
		 * this.
		 */
		if (memsz > filesz)
			memset(ptr + filesz, 0, memsz - filesz);
	}

	return ret;
}

static struct resource_table *sunxi_rproc_rsc_table(struct rproc *rproc,
					     const struct firmware *fw)
{
	dev_dbg(&rproc->dev, "%s,%d\n", __func__, __LINE__);
	return rproc_elf_find_loaded_rsc_table(rproc, fw);
}

static u64 suxni_rproc_elf_get_boot_addr(struct rproc *rproc, const struct firmware *fw)
{
	u64 data = 0;
	data = rproc_elf_get_boot_addr(rproc, fw);
	dev_dbg(&rproc->dev, "%s,%d elf boot addr = 0x%llx\n", __func__, __LINE__, data);
	return data;
}

static struct rproc_ops sunxi_rproc_ops = {
	.start		= sunxi_rproc_start,
	.stop		= sunxi_rproc_stop,
	.da_to_va	= sunxi_rproc_da_to_va,
	.kick		= sunxi_rproc_kick,
	.parse_fw	= sunxi_rproc_parse_fw,
	.find_loaded_rsc_table = sunxi_rproc_rsc_table,
	.load		= sunxi_rproc_elf_load_segments,
	.get_boot_addr	= suxni_rproc_elf_get_boot_addr,
};

static const struct of_device_id sunxi_rproc_match[] = {
	{ .compatible = "allwinner,hifi4-rproc", .data = "hifi4" },
	{ .compatible = "allwinner,e906-rproc", .data = "e906" },
};
MODULE_DEVICE_TABLE(of, sunxi_rproc_match);

static int devm_sunxi_rproc_resource_get(struct rproc *rproc, struct platform_device *pdev)
{
	struct device *dev = rproc->dev.parent;
	struct sunxi_rproc *chip = rproc->priv;
	int ret;

	chip->rproc_priv = sunxi_rproc_priv_find(chip->name);
	if (!chip->rproc_priv) {
		dev_err(dev, "find rproc priv error\n");
		return -EINVAL;
	}

	ret = devm_sunxi_rproc_priv_resource_get(chip->rproc_priv, pdev);
	if (ret) {
		dev_err(dev, "resource get error\n");
		return ret;
	}

	return 0;
}

int sunxi_rproc_report_crash(const char *name, enum rproc_crash_type type)
{
	struct sunxi_rproc *chip, *tmp;

	list_for_each_entry_safe(chip, tmp, &sunxi_rproc_list, list) {
		if (!strcmp(chip->rproc->name, name)) {
			rproc_report_crash(chip->rproc, type);
			return 0;
		}
	}

	return -ENXIO;
}
EXPORT_SYMBOL(sunxi_rproc_report_crash);

static int sunxi_rproc_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	const struct of_device_id *of_id;
	struct sunxi_rproc *chip;
	struct device_node *np = dev->of_node;
	struct rproc *rproc;
	int ret;

	dev_info(dev, "sunxi rproc driver %s\n", SUNXI_RPROC_VERSION);

	of_id = of_match_device(sunxi_rproc_match, dev);
	if (!of_id) {
		dev_err(dev, "No device of_id found\n");
		ret = -EINVAL;
		goto err_out;
	}

	rproc = rproc_alloc(dev, np->name, &sunxi_rproc_ops, NULL, sizeof(*chip));
	if (!rproc) {
		ret = -ENOMEM;
		goto err_out;
	}

	rproc->has_iommu = false;
	rproc->auto_boot = false;
	chip = rproc->priv;
	chip->rproc = rproc;
	chip->name = (char *)of_id->data;

	ret = devm_sunxi_rproc_resource_get(rproc, pdev);
	if (ret) {
		dev_err(dev, "Failed to get resource\n");
		goto free_rproc;
	}

	chip->workqueue = create_workqueue(dev_name(dev));
	if (!chip->workqueue) {
		dev_err(dev, "Cannot create workqueue\n");
		ret = -ENOMEM;
		goto free_rproc;
	}

	platform_set_drvdata(pdev, rproc);

	ret = sunxi_rproc_request_mbox(rproc);
	if (ret) {
		dev_err(dev, "Request mbox failed\n");
		goto destroy_workqueue;
	}

	ret = rproc_add(rproc);
	if (ret) {
		dev_err(dev, "Failed to register rproc\n");
		goto free_mbox;
	}

	list_add(&chip->list, &sunxi_rproc_list);

	dev_info(dev, "sunxi rproc driver probe ok\n");

	return ret;

free_mbox:
	sunxi_rproc_free_mbox(rproc);
destroy_workqueue:
	destroy_workqueue(chip->workqueue);
free_rproc:
	rproc_free(rproc);
err_out:
	return ret;
}

static int sunxi_rproc_remove(struct platform_device *pdev)
{
	struct rproc *rproc = platform_get_drvdata(pdev);
	struct sunxi_rproc *chip = rproc->priv;

	if (atomic_read(&rproc->power) > 0)
		rproc_shutdown(rproc);

	rproc_del(rproc);

	sunxi_rproc_free_mbox(rproc);

	destroy_workqueue(chip->workqueue);

	rproc_free(rproc);

	list_del(&chip->list);

	if (!list_empty(&sunxi_rproc_list))
		list_del(&sunxi_rproc_list);

	return 0;
}

static struct platform_driver sunxi_rproc_driver = {
	.probe = sunxi_rproc_probe,
	.remove = sunxi_rproc_remove,
	.driver = {
		.name = "sunxi-rproc", /* dev name */
		.of_match_table = sunxi_rproc_match,
	},
};
module_platform_driver(sunxi_rproc_driver);

MODULE_DESCRIPTION("Allwinnertech Remote Processor Control Driver");
MODULE_AUTHOR("wujiayi <wujiayi@allwinnertech.com>");
MODULE_AUTHOR("xuminghui <xuminghui@allwinnertech.com>");
MODULE_LICENSE("GPL v2");
MODULE_VERSION(SUNXI_RPROC_VERSION);
