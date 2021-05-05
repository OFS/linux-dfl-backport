/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __BACKPORT_PCI_REGS_H
#define __BACKPORT_PCI_REGS_H

#include_next <uapi/linux/pci_regs.h>

#ifndef PCI_STD_NUM_BARS
#define PCI_STD_NUM_BARS       6       /* Number of standard BARs */
#endif

#endif
