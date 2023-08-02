/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2024 Intel Corporation
 *
 * This file contains macros for maintaining compatibility with older versions
 * of the Linux kernel.
 */

#ifndef _BACKPORT_LINUX_PCI_H_
#define _BACKPORT_LINUX_PCI_H_

#include <linux/version.h>
#include_next <linux/pci.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0)
/* Backwards compatibility prior to commit c124fd9a969a ("PCI: Add
 * pci_find_vsec_capability() to find a specific VSEC").
 */
static inline u16 pci_find_vsec_capability(struct pci_dev *dev, u16 vendor, int cap)
{
	u16 vsec = 0;
	u32 header;

	if (vendor != dev->vendor)
		return 0;

	while ((vsec = pci_find_next_ext_capability(dev, vsec,
						     PCI_EXT_CAP_ID_VNDR))) {
		if (pci_read_config_dword(dev, vsec + PCI_VNDR_HEADER,
					  &header) == PCIBIOS_SUCCESSFUL &&
		    PCI_VNDR_HEADER_ID(header) == cap)
			return vsec;
	}

	return 0;
}
#endif

#endif /* _BACKPORT_LINUX_PCI_H_ */
