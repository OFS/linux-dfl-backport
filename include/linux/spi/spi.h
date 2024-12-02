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

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 3, 0) &&        \
	(LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 71) ||  \
	 LINUX_VERSION_CODE >= KERNEL_VERSION(6, 2, 0)) && \
	RHEL_RELEASE_CODE < 0x905
/* spi_get_chipselect() was introduced in commit 303feb3cc06a ("spi: Add APIs
 * in spi core to set/get spi->chip_select and spi->cs_gpiod").
 */
static inline u8 spi_get_chipselect(const struct spi_device *spi, u8 idx)
{
	return spi->chip_select;
}
#endif

/* spi_alloc_host() and devm_spi_alloc_host() were introduced in commit
 * b8d3b056a78d ("spi: introduce new helpers with using modern naming").
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0) && RHEL_RELEASE_CODE < 0x905
#define spi_alloc_host spi_alloc_master
#define devm_spi_alloc_host devm_spi_alloc_master
#endif

#endif /* _BACKPORT_LINUX_SPI_SPI_H_ */
