/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __BACKPORT_DEV_PRINTK_H
#define __BACKPORT_DEV_PRINTK_H

#include <linux/version.h>


#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 6, 0)
#include <linux/device.h>
#else
#include_next <linux/dev_printk.h>
#endif

#endif
