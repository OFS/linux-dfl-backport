/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __BACKPORT_REGMAP_H
#define __BACKPORT_REGMAP_H

#include <linux/iopoll.h>
#include <linux/version.h>
#include_next <linux/regmap.h>

/**
 * devm_regmap_init_indirect_register - create a regmap for indirect register access
 * @dev: device creating the regmap
 * @base: __iomem point to base of memory with mailbox
 * @cfg: regmap_config describing interface
 *
 * Return: 0 on success, negative error code otherwise.
 */
struct regmap *devm_regmap_init_indirect_register(struct device *dev,
						  void __iomem *base,
						  struct regmap_config *cfg);

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)

struct regmap *__devm_regmap_init_spi_avmm(struct spi_device *spi,
					   const struct regmap_config *config,
					   struct lock_class_key *lock_key,
					   const char *lock_name);

/**
 * regmap_init_spi_avmm() - Initialize register map for Intel SPI Slave
 * to AVMM Bus Bridge
 *
 * @spi: Device that will be interacted with
 * @config: Configuration for register map
 *
 * The return value will be an ERR_PTR() on error or a valid pointer
 * to a struct regmap.
 */
#define regmap_init_spi_avmm(spi, config)					\
	__regmap_lockdep_wrapper(__regmap_init_spi_avmm, #config,		\
				 spi, config)

/**
 * devm_regmap_init_spi_avmm() - Initialize register map for Intel SPI Slave
 * to AVMM Bus Bridge
 *
 * @spi: Device that will be interacted with
 * @config: Configuration for register map
 *
 * The return value will be an ERR_PTR() on error or a valid pointer
 * to a struct regmap.  The map will be automatically freed by the
 * device management code.
 */
#define devm_regmap_init_spi_avmm(spi, config)				\
	__regmap_lockdep_wrapper(__devm_regmap_init_spi_avmm, #config,	\
				 spi, config)

#endif /* < KERNEL_VERSION(5, 10, 0) */

#endif
