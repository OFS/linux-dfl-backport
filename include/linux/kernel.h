/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2020  Silicom Denmark A/S:
 *
 * This file contains macros for maintaining compatibility with older versions
 * of the Linux kernel.
 */

#ifndef _BACKPORT_LINUX_KERNEL_H_
#define _BACKPORT_LINUX_KERNEL_H_

#include <linux/version.h>
#include_next <linux/kernel.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 1, 0)

#include <uapi/linux/limits.h>

#endif /* < KERNEL_VERSION(5, 1, 0) */

#endif /* _BACKPORT_LINUX_KERNEL_H_ */

