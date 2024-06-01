/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __BACKPORT_MINMAX_H
#define __BACKPORT_MINMAX_H

#include <linux/version.h>


#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0) && RHEL_RELEASE_CODE <= 0x804
#include <linux/kernel.h>
#else
#include_next <linux/minmax.h>
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
static inline bool in_range64(u64 val, u64 start, u64 len)
{
	return (val - start) < len;
}

static inline bool in_range32(u32 val, u32 start, u32 len)
{
	return (val - start) < len;
}

#define in_range(val, start, len)					\
	((sizeof(start) | sizeof(len) | sizeof(val)) <= sizeof(u32) ?	\
		in_range32(val, start, len) : in_range64(val, start, len))
#endif

#endif
