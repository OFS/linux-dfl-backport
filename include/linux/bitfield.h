/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __BACKPORT_BITFIELD_H
#define __BACKPORT_BITFIELD_H

#include <linux/version.h>
#include_next <linux/bitfield.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0) && RHEL_RELEASE_CODE <= 0x804
/**
 * FIELD_MAX() - produce the maximum value representable by a field
 * @_mask: shifted mask defining the field's length and position
 *
 * FIELD_MAX() returns the maximum value that can be held in the field
 * specified by @_mask.
 */
#define FIELD_MAX(_mask)                                                \
        ({                                                              \
                __BF_FIELD_CHECK(_mask, 0ULL, 0ULL, "FIELD_MAX: ");     \
                (typeof(_mask))((_mask) >> __bf_shf(_mask));            \
        })
#endif

#endif
