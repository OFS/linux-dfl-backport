/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __BACKPORT_EXPORT_H
#define __BACKPORT_EXPORT_H

#include <linux/version.h>
#include_next <linux/export.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0) && RHEL_RELEASE_CODE < 0x900
#define EXPORT_SYMBOL_NS_GPL(sym, ns)   EXPORT_SYMBOL_GPL(sym)
#endif

#endif
