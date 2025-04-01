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
#include <linux/platform_device.h>
#include <linux/module.h>
#include "sunxi_rproc_boot.h"

#define RPROC_TYPE_MAX		4	/* hifi4/hifi5/c906/e907 */
static struct sunxi_rproc_priv rproc_privs[RPROC_TYPE_MAX];

struct sunxi_rproc_priv *sunxi_rproc_priv_find(const char *name)
{
	int i;
	int rproc_cnt = ARRAY_SIZE(rproc_privs);

	for (i = 0; i < rproc_cnt; i++) {
		if (!strcmp(rproc_privs[i].name, name))
			return &rproc_privs[i];
	}

	return NULL;
}
EXPORT_SYMBOL(sunxi_rproc_priv_find);

static struct sunxi_rproc_priv *sunxi_rproc_priv_find_idle(void)
{
	int i;
	int rproc_cnt = ARRAY_SIZE(rproc_privs);

	for (i = 0; i < rproc_cnt; i++) {
		if (!rproc_privs[i].name)
			return &rproc_privs[i];
	}

	return NULL;
}

int sunxi_rproc_priv_ops_register(const char *name, struct sunxi_rproc_ops *rproc_ops, void *priv)
{
	struct sunxi_rproc_priv *rproc_priv;

	rproc_priv = sunxi_rproc_priv_find_idle();
	if (!rproc_priv)
		return -EBUSY;

	rproc_priv->name = name;
	rproc_priv->ops = rproc_ops;
	rproc_priv->priv = priv;

	return 0;
}
EXPORT_SYMBOL(sunxi_rproc_priv_ops_register);

int sunxi_rproc_priv_ops_unregister(const char *name)
{
	struct sunxi_rproc_priv *rproc_priv;

	rproc_priv = sunxi_rproc_priv_find(name);
	if (!rproc_priv)
		return -EBUSY;

	rproc_priv->name = NULL;
	rproc_priv->ops = NULL;
	rproc_priv->priv = NULL;

	return 0;
}
EXPORT_SYMBOL(sunxi_rproc_priv_ops_unregister);

int devm_sunxi_rproc_priv_resource_get(struct sunxi_rproc_priv *rproc_priv, struct platform_device *pdev)
{
	return rproc_priv->ops->resource_get(rproc_priv, pdev);
}
EXPORT_SYMBOL(devm_sunxi_rproc_priv_resource_get);

int sunxi_rproc_priv_start(struct sunxi_rproc_priv *rproc_priv)
{
	return rproc_priv->ops->start(rproc_priv);
}
EXPORT_SYMBOL(sunxi_rproc_priv_start);

int sunxi_rproc_priv_stop(struct sunxi_rproc_priv *rproc_priv)
{
	return rproc_priv->ops->stop(rproc_priv);
}
EXPORT_SYMBOL(sunxi_rproc_priv_stop);

int sunxi_rproc_priv_reset(struct sunxi_rproc_priv *rproc_priv)
{
	return rproc_priv->ops->reset(rproc_priv);
}
EXPORT_SYMBOL(sunxi_rproc_priv_reset);

int sunxi_rproc_priv_set_localram(struct sunxi_rproc_priv *rproc_priv, u32 value)
{
	return rproc_priv->ops->set_localram(rproc_priv, value);
}
EXPORT_SYMBOL(sunxi_rproc_priv_set_localram);

int sunxi_rproc_priv_set_runstall(struct sunxi_rproc_priv *rproc_priv, u32 value)
{
	return rproc_priv->ops->set_runstall(rproc_priv, value);
}
EXPORT_SYMBOL(sunxi_rproc_priv_set_runstall);

MODULE_DESCRIPTION("Allwinner sunxi rproc boot driver");
MODULE_AUTHOR("xuminghui <xuminghui@allwinnertech.com>");
MODULE_LICENSE("GPL v2");
