/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __BACKPORT_UNITS_H
#define __BACKPORT_UNITS_H

#include <linux/version.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0) && RHEL_RELEASE_CODE <= 0x902
#define GIGA    1000000000UL
#define MEGA    1000000UL
#define MICRO   1000000UL
#define MILLI	1000UL
#else
#include_next <linux/units.h>
#endif

#endif
