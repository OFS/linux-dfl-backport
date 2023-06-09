/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __BACKPORT_VERSION_H
#define __BACKPORT_VERSION_H

#include_next <linux/version.h>

#if !defined(RHEL_RELEASE_CODE)
#define RHEL_RELEASE_CODE 0
#endif

#endif
