/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2020  Silicom Denmark A/S:
 *
 * This file contains macros for maintaining compatibility with older versions
 * of the Linux kernel.
 */

#ifndef _BACKPORT_LINUX_COMPILER_TYPES_H_
#define _BACKPORT_LINUX_COMPILER_TYPES_H_

#include <linux/version.h>
#include_next <linux/compiler_types.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 5, 0)

#if __has_attribute(__fallthrough__)
# define fallthrough                    __attribute__((__fallthrough__))
#else
# define fallthrough                    do {} while (0)  /* fallthrough */
#endif


#endif /* < KERNEL_VERSION(5, 5, 0) */

#endif /* _BACKPORT_LINUX_COMPILER_TYPES_H_ */
