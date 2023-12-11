/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2021 Silicom Denmark A/S
 * Copyright (C) 2023 Intel Corporation
 *
 * This file contains macros for maintaining compatibility with older versions
 * of the Linux kernel.
 */

#ifndef _BACKPORT_LINUX_MM_H_
#define _BACKPORT_LINUX_MM_H_

#include <linux/version.h>

#include_next <linux/mm.h>

/* Explicit pinning of user-space pages was introduced in 5.6.
 * https://lwn.net/Articles/807108/
 * https://lwn.net/Kernel/Index/#Memory_management-get_user_pages
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 6, 0) && RHEL_RELEASE_CODE < 0x804
#define pin_user_pages_fast get_user_pages_fast
#define unpin_user_pages put_user_pages
#endif

/* put_user_pages() was introduced in 5.2 and removed in 5.6. */
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 2, 0) && RHEL_RELEASE_CODE < 0x802
static inline void put_user_pages(struct page **pages, unsigned long npages)
{
	unsigned long index;

	for (index = 0; index < npages; index++)
		put_page(pages[index]);
}
#endif

#endif /* _BACKPORT_LINUX_MM_H_ */
