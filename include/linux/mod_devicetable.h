/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2024 Intel Corporation
 *
 * This file contains macros for maintaining compatibility with older versions
 * of the Linux kernel.
 */

#ifndef BACKPORT_LINUX_MOD_DEVICETABLE_H
#define BACKPORT_LINUX_MOD_DEVICETABLE_H

#include <linux/version.h>
#include_next <linux/mod_devicetable.h>

/*
 * DFL (Device Feature List)
 *
 * DFL defines a linked list of feature headers within the device MMIO space to
 * provide an extensible way of adding features. Software can walk through these
 * predefined data structures to enumerate features. It is now used in the FPGA.
 * See Documentation/fpga/dfl.rst for more information.
 *
 * The dfl bus type is introduced to match the individual feature devices (dfl
 * devices) for specific dfl drivers.
 */

/**
 * struct dfl_device_id -  dfl device identifier
 * @type: DFL FIU type of the device. See enum dfl_id_type.
 * @feature_id: feature identifier local to its DFL FIU type.
 * @driver_data: driver specific data.
 * @guid_string: 36 char string of the form 03020100-0504-0706-0809-0A0B0C0D0E0F
 */
#define dfl_device_id BACKPORT_dfl_device_id
struct BACKPORT_dfl_device_id {
	__u16 type;
	__u16 feature_id;
	kernel_ulong_t driver_data;
	const char guid_string[UUID_STRING_LEN + 1];
};

#endif /* BACKPORT_LINUX_MOD_DEVICETABLE_H */
