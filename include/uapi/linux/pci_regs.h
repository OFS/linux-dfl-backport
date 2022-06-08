/* SPDX-License-Identifier: GPL-2.0 */

#ifndef BACKPORT_PCI_REGS_H
#define BACKPORT_PCI_REGS_H

#include_next <linux/pci_regs.h>

#ifndef PCI_STD_NUM_BARS
#define PCI_STD_NUM_BARS	6	/* Number of standard BARs */
#endif

#endif
