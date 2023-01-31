/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __BACKPORT_MINMAX_H
#define __BACKPORT_MINMAX_H

#include <linux/version.h>


#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
#include <linux/kernel.h>
#else
#include_next <linux/minmax.h>
#endif

#endif
