/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2024 Intel Corporation
 *
 * This file contains macros for maintaining compatibility with older versions
 * of the Linux kernel.
 */

#ifndef _BACKPORT_LINUX_SPI_SPI_H_
#define _BACKPORT_LINUX_SPI_SPI_H_

#include <linux/version.h>
#include_next <linux/spi/spi.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 3, 0) &&       \
	(LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 71) || \
	 LINUX_VERSION_CODE >= KERNEL_VERSION(6, 2, 0))
/* spi_get_chipselect() was introduced in commit 303feb3cc06a ("spi: Add APIs
 * in spi core to set/get spi->chip_select and spi->cs_gpiod").
 */
static inline u8 spi_get_chipselect(const struct spi_device *spi, u8 idx)
{
	return spi->chip_select;
}
#endif

#endif /* _BACKPORT_LINUX_SPI_SPI_H_ */
