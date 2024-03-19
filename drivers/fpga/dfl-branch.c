// SPDX-License-Identifier: GPL-2.0
/*
 * Driver for FPGA BRANCH DFL
 *
 * Copyright (C) 2023 Intel Corporation.
 *
 * Authors:
 *   Basheer Ahmed Muddebihal <basheer.ahmed.muddebihal@linux.intel.com>
 */

#include <linux/bitfield.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/ioport.h>
#include <linux/module.h>
#include <linux/mod_devicetable.h>
#include <linux/types.h>

#include "dfl.h"

#define DRV_NAME	"dfl-branch"

#define DFHv1_PARAM_ID_BRANCH_DFL	0xF00C
#define DFL_BRANCH_SIZE			0x2000	/*default DFL ROM Size*/
#define DFL_BRANCH_PARAM_DATA_LEN	16	/*two 8-byte words*/

/*
 *  REL_N, 1'b0 = relative (offset from feature DFH start),
 * 1'b1 = absolute (ARM or other non-PCIe use)
 */
#define DFHv1_BRANCH_DFL_ADDR_REL_N	BIT_ULL(0)
#define DFHv1_BRANCH_DFL_ADDR		GENMASK_ULL(63, 3)  /* 63:3 of DFL address */
#define DFHv1_BRANCH_DFL_SIZE		GENMASK_ULL(63, 32) /* 63:32 size of DFL */

static int dfh_branch_get_param_vals(struct dfl_device *dfl_dev, int param_id,
				     u64 *bdfl_addr, size_t *bdfl_size, bool *rel_addr)
{
	struct device *dev = &dfl_dev->dev;
	u64 addr_val, size_val;
	size_t psize;
	u64 *p;

	p = dfh_find_param(dfl_dev, param_id, &psize);
	if (IS_ERR(p))
		return PTR_ERR(p);

	if (psize == DFL_BRANCH_PARAM_DATA_LEN) {
		addr_val = *p;
		size_val = *(p + 1);
		dev_info(dev, "addr_val = %pa, size_val=%pa\n", &addr_val, &size_val);
	} else {
		dev_err(dev, "Branch parameters data is missing\n");
		return -EINVAL;
	}

	if (FIELD_GET(DFHv1_BRANCH_DFL_ADDR_REL_N, addr_val))
		*rel_addr = false;
	else
		*rel_addr = true;

	*bdfl_addr = FIELD_GET(DFHv1_BRANCH_DFL_ADDR, addr_val) << 3;
	*bdfl_size = FIELD_GET(DFHv1_BRANCH_DFL_SIZE, size_val);

	return 0;
}

static int dfl_branch_process_dfl(struct dfl_device *dfl_dev)
{
	struct device *dev = &dfl_dev->dev;
	struct dfl_fpga_enum_info *info;
	struct dfl_fpga_cdev *cdev;
	resource_size_t start, len;
	resource_size_t feat_base;
	bool rel_addr = true;
	u64 *pstart, *pend;
	size_t bdfl_size;
	u64 bdfl_addr;
	u64 next, v;
	int ret;

	/* allocate enumeration info  */
	info = dfl_fpga_enum_info_alloc(dev);
	if (!info)
		return -ENOMEM;

	pstart = dfl_dev->params;
	pend = dfl_dev->params + dfl_dev->param_size / sizeof(u64);

	feat_base = dfl_dev->mmio_res.start;

	while (pstart < pend) {
		v = *pstart;

		ret = dfh_branch_get_param_vals(dfl_dev, DFHv1_PARAM_ID_BRANCH_DFL,
						&bdfl_addr, &bdfl_size, &rel_addr);

		if (!ret) {
			if (rel_addr) {
				start = feat_base + bdfl_addr;
				dev_info(dev, "Start=%pa\n", &start);
			} else {
				start = bdfl_addr;
			}

			len = bdfl_size ? bdfl_size : DFL_BRANCH_SIZE;
			dfl_fpga_enum_info_add_dfl(info, start, len);
		}
		next = FIELD_GET(DFHv1_PARAM_HDR_NEXT_OFFSET, v);
		pstart += next;
	}

	/* start enumeration with prepared enumeration information */
	cdev = dfl_fpga_feature_devs_enumerate(info);
	if (IS_ERR(cdev)) {
		dev_err(&dfl_dev->dev, "Enumeration failure\n");
		ret = PTR_ERR(cdev);
		goto info_free_exit;
	}

	dfl_dev->cdev = cdev;

info_free_exit:
	dfl_fpga_enum_info_free(info);

	return ret;
}

static int dfl_branch_probe(struct dfl_device *dfl_dev)
{
	struct device *dev = &dfl_dev->dev;
	int ret;

	ret = dfl_branch_process_dfl(dfl_dev);
	if (ret < 0)
		return dev_err_probe(dev, ret, "Failed to find the branch dfl\n");

	return 0;
}

static void dfl_branch_remove(struct dfl_device *dfl_dev)
{
	dfl_fpga_feature_devs_remove(dfl_dev->cdev);
}

#define DFL_BRANCH_GUID "30C45AEA-68F6-42E6-AEFB-15B4B5E28284"

static const struct dfl_device_id dfl_branch_ids[] = {
	{ .guid_string = DFL_BRANCH_GUID },
	{ }
};
MODULE_DEVICE_TABLE(dfl, dfl_branch_ids);

static struct dfl_driver dfl_branch_driver = {
	.drv = {
		.name = "dfl-branch",
	},
	.id_table = dfl_branch_ids,
	.probe = dfl_branch_probe,
	.remove = dfl_branch_remove,
};
module_dfl_driver(dfl_branch_driver);

MODULE_ALIAS("dfl:t*f*g{30C45AEA-68F6-42E6-AEFB-15B4B5E28284}");
MODULE_DESCRIPTION("DFL Intel Branch DFL driver");
MODULE_AUTHOR("Intel Corporation");
MODULE_LICENSE("GPL");
