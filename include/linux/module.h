/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __BACKPORT_MODULE_H
#define __BACKPORT_MODULE_H

#include <linux/version.h>
#include_next <linux/module.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0) && RHEL_RELEASE_CODE < 0x900
#define MODULE_IMPORT_NS(ns)
#endif

#endif
