/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2024 Intel Corporation
 *
 * This file contains macros for maintaining compatibility with older versions
 * of the Linux kernel.
 */

#ifndef _BACKPORT_LINUX_EVENTFD_H_
#define _BACKPORT_LINUX_EVENTFD_H_

#include <linux/version.h>
#include_next <linux/eventfd.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 8, 0)
/* Before commit 3652117f8548 ("eventfd: simplify eventfd_signal()")
 * eventfd_signal() received an extra argument @n that was always 1.
 */
#define eventfd_signal(ctx) eventfd_signal(ctx, 1)
#endif

#endif /* _BACKPORT_LINUX_EVENTFD_H_ */
