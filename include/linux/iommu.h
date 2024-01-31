/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2023 Intel Corporation
 *
 * This file contains macros for maintaining compatibility with older versions
 * of the Linux kernel.
 */

#ifndef _BACKPORT_LINUX_IOMMU_H_
#define _BACKPORT_LINUX_IOMMU_H_

#include <linux/version.h>

#include_next <linux/iommu.h>

/* Backwards compatibility prior to commit 942fd5435dcc ("iommu: Remove
 * SVM_FLAG_SUPERVISOR_MODE support") which removes the drvdata parameter.
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 2, 0) &&                            \
    (RHEL_RELEASE_CODE < 0x809 ||                                              \
     (RHEL_RELEASE_CODE >= 0x900 && RHEL_RELEASE_CODE < 0x903))
#define iommu_sva_bind_device(dev, mm) iommu_sva_bind_device(dev, mm, NULL)
#endif

/* Commit 2396046d75d3 ("iommu: Add mm_get_enqcmd_pasid() helper function")
 * added a wrapper for pasid, which was moved to a private data structure in
 * commit 092edaddb660 ("iommu: Support mm PASID 1:n with sva domains").
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 8, 0)
static inline u32 mm_get_enqcmd_pasid(struct mm_struct *mm)
{
	return mm->pasid;
}
#endif

#endif /* _BACKPORT_LINUX_IOMMU_H_ */
