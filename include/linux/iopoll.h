// SPDX-License-Identifier: GPL-2.0

#ifndef _BACKPORT_IOPOLL_H_
#define _BACKPORT_IOPOLL_H_

#include <linux/version.h>
#include_next <linux/iopoll.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)

#define read_poll_timeout(op, val, cond, sleep_us, timeout_us, \
				sleep_before_read, args...) \
({ \
	u64 __timeout_us = (timeout_us); \
	unsigned long __sleep_us = (sleep_us); \
	ktime_t __timeout = ktime_add_us(ktime_get(), __timeout_us); \
	might_sleep_if((__sleep_us) != 0); \
	if (sleep_before_read && __sleep_us) \
		usleep_range((__sleep_us >> 2) + 1, __sleep_us); \
	for (;;) { \
		(val) = op(args); \
		if (cond) \
			break; \
		if (__timeout_us && \
		    ktime_compare(ktime_get(), __timeout) > 0) { \
			(val) = op(args); \
			break; \
		} \
		if (__sleep_us) \
			usleep_range((__sleep_us >> 2) + 1, __sleep_us); \
	} \
	(cond) ? 0 : -ETIMEDOUT; \
})

#endif /* < KERNEL_VERSION(5, 7, 0) */

#endif
