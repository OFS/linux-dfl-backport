/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/*
 * Header File for XYZ DFL User API
 *
 * Copyright (C) 2023 Intel Corporation, Inc.
 *
 * Authors:
 *   Tim Whisonant <tim.whisonant@intel.com>
 *   Ananda Ravuri <ananda.ravuri@intel.com>
 *   Russell H. Weight <russell.h.weight@intel.com>
 */

#ifndef _UAPI_LINUX_XYZ_DFL_H
#define _UAPI_LINUX_XYZ_DFL_H

#include <linux/types.h>
#include <linux/ioctl.h>

#define DFL_XYZ_API_VERSION 0

/*
 * The IOCTL interface for DFL based XYZ is designed for extensibility by
 * embedding the structure length (argsz) and flags into structures passed
 * between kernel and userspace. This design referenced the VFIO IOCTL
 * interface (include/uapi/linux/vfio.h).
 */

#define DFL_XYZ_MAGIC 0xB6

#define DFL_XYZ_BASE 0


/**
 * DFL_FPGA_GET_API_VERSION - _IO(DFL_FPGA_MAGIC, DFL_FPGA_BASE + 0)
 *
 * Report the version of the driver API.
 * Return: Driver API Version.
 */

#define DFL_XYZ_GET_API_VERSION	_IO(DFL_XYZ_MAGIC, DFL_XYZ_BASE + 0)

/**
 * DFL_FPGA_CHECK_EXTENSION - _IO(DFL_FPGA_MAGIC, DFL_FPGA_BASE + 1)
 *
 * Check whether an extension is supported.
 * Return: 0 if not supported, otherwise the extension is supported.
 */

#define DFL_XYZ_CHECK_EXTENSION	_IO(DFL_XYZ_MAGIC, DFL_XYZ_BASE + 1)



#define DFL_XYZ_GET_REGION_INFO _IO(DFL_XYZ_MAGIC, DFL_XYZ_BASE + 2)

/**
* FPGA_PORT_GET_REGION_INFO - _IOWR(FPGA_MAGIC, PORT_BASE + 2,
*                                      struct dfl_xyz_region_info)
*
* Retrieve information about a device memory region.
* Caller provides struct dfl_fpga_port_region_info with index value set.
* Driver returns the region info in other fields.
* Return: 0 on success, -errno on failure.
*/
struct dfl_xyz_region_info {
	/* Input */
	__u32 argsz;            /* Structure length */
	/* Output */
	__u32 flags;            /* Access permission */
#define DFL_XYZ_REGION_READ	(1 << 0)        /* Region is readable */
#define DFL_XYZ_REGION_WRITE	(1 << 1)        /* Region is writable */
#define DFL_XYZ_REGION_MMAP	(1 << 2)        /* Can be mmaped to userspace */
	__u64 size;             /* Region size (bytes) */
	__u64 offset;           /* Region offset from start of device fd */
};


#define DFL_XYZ_SET_DSM_INFO _IO(DFL_XYZ_MAGIC, DFL_XYZ_BASE + 3)

struct dfl_xyz_dsm_info {
	/* Input */
	__u32 argsz;		/* Structure length */
	__u64 user_addr;	/* Process virtual address */
	__u64 length;		/* Length of mapping (bytes)*/
};


#define DFL_XYZ_CLEAR_DSM_INFO _IO(DFL_XYZ_MAGIC, DFL_XYZ_BASE + 4)


#define DFL_XYZ_ALLOC_ADDR_TABLE	_IO(DFL_XYZ_MAGIC, DFL_XYZ_BASE + 5)
#define DFL_XYZ_FREE_ADDR_TABLE		_IO(DFL_XYZ_MAGIC, DFL_XYZ_BASE + 6)
#define DFL_XYZ_APPEND_ADDR_TABLE	_IO(DFL_XYZ_MAGIC, DFL_XYZ_BASE + 7)

#define DFL_XYZ_NUM_LINES_MIN 1
#define DFL_XYZ_NUM_LINES_MAX 0xffff

struct dfl_xyz_addr_table {
	/* Input */
	__u32 argsz;		/* Structure length */
	__u32 flags;		/* Address Table ID */
#define DFL_XYZ_READ_ADDR_TABLE		(1 << 0)
#define DFL_XYZ_WRITE_ADDR_TABLE	(1 << 1)
	__u32 cache_lines;	/* Buffer size/offset in cache lines */
};


#endif /* _UAPI_LINUX_XYZ_DFL_H */
