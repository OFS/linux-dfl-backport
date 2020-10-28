// SPDX-License-Identifier: GPL-2.0
/*
 * Intel Max10 Board Management Controller Secure Update Driver
 *
 * Copyright (C) 2019-2020 Intel Corporation. All rights reserved.
 *
 */
#include <linux/bitfield.h>
#include <linux/device.h>
#include <linux/fpga/fpga-sec-mgr.h>
#include <linux/mfd/intel-m10-bmc.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>

struct m10bmc_sec {
	struct device *dev;
	struct intel_m10bmc *m10bmc;
};

/* Root Entry Hash (REH) support */
#define REH_SHA256_SIZE		32
#define REH_SHA384_SIZE		48
#define REH_MAGIC		GENMASK(15, 0)
#define REH_SHA_NUM_BYTES	GENMASK(31, 16)

static int m10bmc_reh_size(struct fpga_sec_mgr *smgr,
			   u32 exp_magic, u32 prog_addr)
{
	struct m10bmc_sec *sec = smgr->priv;
	int sha_num_bytes, ret;
	u32 magic;

	ret = m10bmc_raw_read(sec->m10bmc, prog_addr, &magic);
	if (ret)
		return ret;

	dev_dbg(sec->dev, "%s magic 0x%08x\n", __func__, magic);

	/*
	 * If no magic number, then no Root Entry Hash (REH) is programmed,
	 * so the REH size is zero.
	 */
	if (FIELD_GET(REH_MAGIC, magic) != exp_magic)
		return 0;

	sha_num_bytes = FIELD_GET(REH_SHA_NUM_BYTES, magic) / 8;
	if (sha_num_bytes != REH_SHA256_SIZE &&
	    sha_num_bytes != REH_SHA384_SIZE)   {
		dev_err(sec->dev, "%s bad sha num bytes %d\n", __func__,
			sha_num_bytes);
		return -EINVAL;
	}

	return sha_num_bytes;
}

static int m10bmc_bmc_reh_size(struct fpga_sec_mgr *smgr)
{
	return m10bmc_reh_size(smgr, BMC_PROG_MAGIC, BMC_PROG_ADDR);
}

static int m10bmc_sr_reh_size(struct fpga_sec_mgr *smgr)
{
	return m10bmc_reh_size(smgr, SR_PROG_MAGIC, SR_PROG_ADDR);
}

static int m10bmc_pr_reh_size(struct fpga_sec_mgr *smgr)
{
	return m10bmc_reh_size(smgr, PR_PROG_MAGIC, PR_PROG_ADDR);
}

static int m10bmc_reh(struct fpga_sec_mgr *smgr, u32 hash_addr,
		      u8 *hash, unsigned int size)
{
	struct m10bmc_sec *sec = smgr->priv;
	unsigned int stride = regmap_get_reg_stride(sec->m10bmc->regmap);
	int ret;

	ret = regmap_bulk_read(sec->m10bmc->regmap, hash_addr,
			       hash, size / stride);
	if (ret)
		dev_err(sec->m10bmc->dev,
			"failed to read root entry hash: %x cnt %x: %d\n",
			hash_addr, size / stride, ret);

	return ret;
}

static int m10bmc_bmc_reh(struct fpga_sec_mgr *smgr, u8 *hash,
			  unsigned int size)
{
	return m10bmc_reh(smgr, BMC_REH_ADDR, hash, size);
}

static int m10bmc_sr_reh(struct fpga_sec_mgr *smgr, u8 *hash,
			 unsigned int size)
{
	return m10bmc_reh(smgr, SR_REH_ADDR, hash, size);
}

static int m10bmc_pr_reh(struct fpga_sec_mgr *smgr, u8 *hash,
			 unsigned int size)
{
	return m10bmc_reh(smgr, PR_REH_ADDR, hash, size);
}

#define FLASH_COUNT_SIZE 4096	/* count stored in inverted bit vector */

static int m10bmc_user_flash_count(struct fpga_sec_mgr *smgr)
{
	struct m10bmc_sec *sec = smgr->priv;
	unsigned int stride = regmap_get_reg_stride(sec->m10bmc->regmap);
	unsigned int num_bits = FLASH_COUNT_SIZE * 8;
	u8 *flash_buf;
	int ret;

	flash_buf = kmalloc(FLASH_COUNT_SIZE, GFP_KERNEL);
	if (!flash_buf)
		return -ENOMEM;

	ret = regmap_bulk_read(sec->m10bmc->regmap, USER_FLASH_COUNT,
			       flash_buf, FLASH_COUNT_SIZE / stride);
	if (ret) {
		dev_err(sec->dev,
			"failed to read flash count: %x cnt %x: %d\n",
			USER_FLASH_COUNT, FLASH_COUNT_SIZE / stride, ret);
		goto exit_free;
	}

	ret = num_bits - bitmap_weight((unsigned long *)flash_buf, num_bits);

exit_free:
	kfree(flash_buf);

	return ret;
}

#define CSK_BIT_LEN			128U
#define CSK_32ARRAY_SIZE(_nbits)	DIV_ROUND_UP(_nbits, 32)

static int m10bmc_csk_cancel_nbits(struct fpga_sec_mgr *smgr)
{
	return (int)CSK_BIT_LEN;
}

static int m10bmc_csk_vector(struct fpga_sec_mgr *smgr, u32 addr,
			     unsigned long *csk_map, unsigned int nbits)
{
	unsigned int i, size, arr_size = CSK_32ARRAY_SIZE(nbits);
	struct m10bmc_sec *sec = smgr->priv;
	unsigned int stride;
	__le32 *csk_le32;
	u32 *csk32;
	int ret;

	stride = regmap_get_reg_stride(sec->m10bmc->regmap);
	size = arr_size * sizeof(u32);

	csk32 = vmalloc(size);
	if (!csk32)
		return -ENOMEM;

	csk_le32 = vmalloc(size);
	if (!csk_le32) {
		vfree(csk32);
		return -ENOMEM;
	}

	ret = regmap_bulk_read(sec->m10bmc->regmap, addr, csk_le32, size / stride);
	if (ret) {
		dev_err(sec->dev, "failed to read CSK vector: %x cnt %x: %d\n",
			addr, size / stride, ret);
		goto vfree_exit;
	}

	for (i = 0; i < arr_size; i++)
		csk32[i] = le32_to_cpu(((csk_le32[i])));

	bitmap_from_arr32(csk_map, csk32, nbits);
	bitmap_complement(csk_map, csk_map, nbits);

vfree_exit:
	vfree(csk_le32);
	vfree(csk32);
	return ret;
}

#define CSK_VEC_OFFSET 0x34

static int m10bmc_bmc_canceled_csks(struct fpga_sec_mgr *smgr,
				    unsigned long *csk_map,
				    unsigned int nbits)
{
	return m10bmc_csk_vector(smgr, BMC_PROG_ADDR + CSK_VEC_OFFSET,
				 csk_map, nbits);
}

static int m10bmc_sr_canceled_csks(struct fpga_sec_mgr *smgr,
				   unsigned long *csk_map,
				   unsigned int nbits)
{
	return m10bmc_csk_vector(smgr, SR_PROG_ADDR + CSK_VEC_OFFSET,
				 csk_map, nbits);
}

static int m10bmc_pr_canceled_csks(struct fpga_sec_mgr *smgr,
				   unsigned long *csk_map,
				   unsigned int nbits)
{
	return m10bmc_csk_vector(smgr, PR_PROG_ADDR + CSK_VEC_OFFSET,
				 csk_map, nbits);
}

static const struct fpga_sec_mgr_ops m10bmc_sops = {
	.user_flash_count = m10bmc_user_flash_count,
	.bmc_root_entry_hash = m10bmc_bmc_reh,
	.sr_root_entry_hash = m10bmc_sr_reh,
	.pr_root_entry_hash = m10bmc_pr_reh,
	.bmc_canceled_csks = m10bmc_bmc_canceled_csks,
	.sr_canceled_csks = m10bmc_sr_canceled_csks,
	.pr_canceled_csks = m10bmc_pr_canceled_csks,
	.bmc_reh_size = m10bmc_bmc_reh_size,
	.sr_reh_size = m10bmc_sr_reh_size,
	.pr_reh_size = m10bmc_pr_reh_size,
	.bmc_canceled_csk_nbits = m10bmc_csk_cancel_nbits,
	.sr_canceled_csk_nbits = m10bmc_csk_cancel_nbits,
	.pr_canceled_csk_nbits = m10bmc_csk_cancel_nbits,
};

static int m10bmc_secure_probe(struct platform_device *pdev)
{
	struct fpga_sec_mgr *smgr;
	struct m10bmc_sec *sec;
	int ret;

	sec = devm_kzalloc(&pdev->dev, sizeof(*sec), GFP_KERNEL);
	if (!sec)
		return -ENOMEM;

	sec->dev = &pdev->dev;
	sec->m10bmc = dev_get_drvdata(pdev->dev.parent);
	dev_set_drvdata(&pdev->dev, sec);

	smgr = devm_fpga_sec_mgr_create(sec->dev, "Max10 BMC Secure Update",
					&m10bmc_sops, sec);
	if (!smgr) {
		dev_err(sec->dev,
			"Security manager failed to start: %d\n", ret);
		return -ENOMEM;
	}

	return devm_fpga_sec_mgr_register(sec->dev, smgr);
}

static struct platform_driver intel_m10bmc_secure_driver = {
	.probe = m10bmc_secure_probe,
	.driver = {
		.name = "n3000bmc-secure",
	},
};
module_platform_driver(intel_m10bmc_secure_driver);

MODULE_ALIAS("platform:n3000bmc-secure");
MODULE_AUTHOR("Intel Corporation");
MODULE_DESCRIPTION("Intel MAX10 BMC Secure Update");
MODULE_LICENSE("GPL v2");
