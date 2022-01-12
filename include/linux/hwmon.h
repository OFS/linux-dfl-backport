// SPDX-License-Identifier: GPL-2.0

#ifndef _BACKPORT_HWMON_H_
#define _BACKPORT_HWMON_H_

#include <linux/version.h>
#include_next <linux/hwmon.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 2, 0)

#define HWMON_CHANNEL_INFO(stype, ...)	\
	(&(struct hwmon_channel_info) {	\
		.type = hwmon_##stype,	\
		.config = (u32 []) {	\
			__VA_ARGS__, 0	\
		}			\
	})

#endif /* < KERNEL_VERSION(5, 2, 0) */

#endif
