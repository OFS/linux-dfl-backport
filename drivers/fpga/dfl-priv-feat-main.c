// SPDX-License-Identifier: GPL-2.0-only
/*
 * Driver for FPGA Private Features
 *
 * Copyright (C) 2023 Intel Corp.
 *
 * Authors:
 *   Basheer Ahmed Muddebihal <basheer.ahmed.muddebihal@linux.intel.com>
 */

#include <linux/kernel.h>
#include <linux/module.h>

#include "dfl.h"
#include "dfl-priv-feat.h"

static ssize_t group_id_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct dfl_device *ddev = to_dfl_dev(dev);
	if (!ddev->dfh_version)
		return -ENOENT;

	return sysfs_emit(buf, "0x%X\n", ddev->group_id);
}
static DEVICE_ATTR_RO(group_id);

static ssize_t inst_id_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct dfl_device *ddev = to_dfl_dev(dev);
	if (!ddev->dfh_version)
		return -ENOENT;

	return sysfs_emit(buf, "0x%X\n", ddev->inst_id);
}
static DEVICE_ATTR_RO(inst_id);

static struct attribute *dfl_priv_feat_attrs[] = {
	&dev_attr_group_id.attr,
	&dev_attr_inst_id.attr,
	NULL,
};

static const struct attribute_group dfl_priv_feat_group = {
	.attrs = dfl_priv_feat_attrs,
};

static struct dfl_feature_driver dfl_priv_feat_drvs[] = {
	{
		.id_table = NULL,
		.ops = NULL,
	},
};

static int dfl_priv_feat_dev_init(struct platform_device *pdev)
{
	struct dfl_feature_platform_data *pdata = dev_get_platdata(&pdev->dev);
	struct dfl_feature_dev_data *fdata = pdata->fdata;
	struct dfl_priv_feat *pfeat;

	pfeat = devm_kzalloc(&pdev->dev, sizeof(*pfeat), GFP_KERNEL);
	if (!pfeat)
		return -ENOMEM;

	pfeat->pdata = pdata;

	mutex_lock(&fdata->lock);
	dfl_fpga_fdata_set_private(fdata, pfeat);
	mutex_unlock(&fdata->lock);

	return 0;
}

static void dfl_priv_feat_dev_destroy(struct platform_device *pdev)
{
	struct dfl_feature_platform_data *pdata = dev_get_platdata(&pdev->dev);
	struct dfl_feature_dev_data *fdata = pdata->fdata;

	mutex_lock(&fdata->lock);
	dfl_fpga_fdata_set_private(fdata, NULL);
	mutex_unlock(&fdata->lock);
}

static const struct attribute_group *dfl_priv_feat_dev_groups[] = {
	&dfl_priv_feat_group,
	NULL
};

static int dfl_priv_feat_probe(struct platform_device *pdev)
{
	int ret;

	ret = dfl_priv_feat_dev_init(pdev);
	if (ret)
		goto exit;

	ret = dfl_fpga_dev_feature_init(pdev, dfl_priv_feat_drvs);
	if (ret)
		goto dev_destroy;

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0) && RHEL_RELEASE_CODE < 0x803
	ret = device_add_groups(&pdev->dev, dfl_priv_feat_dev_groups);
	if (ret)
		goto dev_destroy;
#endif

	return 0;

dev_destroy:
	dfl_priv_feat_dev_destroy(pdev);
exit:
	return ret;
}

static int dfl_priv_feat_remove(struct platform_device *pdev)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0) && RHEL_RELEASE_CODE < 0x803
	device_remove_groups(&pdev->dev, dfl_priv_feat_dev_groups);
#endif
	dfl_fpga_dev_feature_uinit(pdev);
	dfl_priv_feat_dev_destroy(pdev);

	return 0;
}

static struct platform_driver dfl_priv_feat_driver = {
	.driver	= {
		.name       = DFL_FPGA_FEATURE_DEV_PRIV_FEAT,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0) || RHEL_RELEASE_CODE >= 0x803
		.dev_groups = dfl_priv_feat_dev_groups,
#endif
	},
	.probe   = dfl_priv_feat_probe,
	.remove  = dfl_priv_feat_remove,
};

module_platform_driver(dfl_priv_feat_driver);

MODULE_DESCRIPTION("FPGA Privare Feature driver");
MODULE_AUTHOR("Intel Corporation");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:dfl-priv-feat");
