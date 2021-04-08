/* SPDX-License-Identifier: GPL-2.0 */

#ifndef BACKPORT_PCI_IDS_H
#define BACKPORT_PCI_IDS_H

#include_next <linux/pci_ids.h>

#ifndef PCI_VENDOR_ID_SILICOM_DENMARK
#define PCI_VENDOR_ID_SILICOM_DENMARK	0x1c2c
#endif

#ifndef PCI_STD_NUM_BARS
#define PCI_STD_NUM_BARS       6       /* Number of standard BARs */
#endif

#endif
