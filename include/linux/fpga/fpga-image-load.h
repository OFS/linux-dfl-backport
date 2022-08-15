/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Header file for FPGA Image Load Framework
 *
 * Copyright (C) 2019-2021 Intel Corporation, Inc.
 */
#ifndef _LINUX_FPGA_IMAGE_LOAD_H
#define _LINUX_FPGA_IMAGE_LOAD_H

#include <linux/device.h>
#include <linux/eventfd.h>
#include <linux/mutex.h>
#include <linux/types.h>
#include <uapi/linux/fpga-image-load.h>

/**
 * enum fw_upload_err - firmware upload error codes
 * @FW_UPLOAD_ERR_NONE: returned to indicate success
 * @FW_UPLOAD_ERR_HW_ERROR: error signalled by hardware, see kernel log
 * @FW_UPLOAD_ERR_TIMEOUT: SW timed out on handshake with HW/firmware
 * @FW_UPLOAD_ERR_CANCELED: upload was cancelled by the user
 * @FW_UPLOAD_ERR_BUSY: there is an upload operation already in progress
 * @FW_UPLOAD_ERR_INVALID_SIZE: invalid firmware image size
 * @FW_UPLOAD_ERR_RW_ERROR: read or write to HW failed, see kernel log
 * @FW_UPLOAD_ERR_WEAROUT: FLASH device is approaching wear-out, wait & retry
 * @FW_UPLOAD_ERR_MAX: Maximum error code marker
 */
enum fw_upload_err {
	FW_UPLOAD_ERR_NONE,
	FW_UPLOAD_ERR_HW_ERROR,
	FW_UPLOAD_ERR_TIMEOUT,
	FW_UPLOAD_ERR_CANCELED,
	FW_UPLOAD_ERR_BUSY,
	FW_UPLOAD_ERR_INVALID_SIZE,
	FW_UPLOAD_ERR_RW_ERROR,
	FW_UPLOAD_ERR_WEAROUT,
	FW_UPLOAD_ERR_MAX
};

struct fw_upload {
	void *dd_handle; /* reference to parent driver */
	void *priv;	 /* firmware loader private fields */
};

/**
 * struct fw_upload_ops - device specific operations to support firmware upload
 * @prepare:		  Required: Prepare secure update
 * @write:		  Required: The write() op receives the remaining
 *			  size to be written and must return the actual
 *			  size written or a negative error code. The write()
 *			  op will be called repeatedly until all data is
 *			  written.
 * @poll_complete:	  Required: Check for the completion of the
 *			  HW authentication/programming process.
 * @cancel:		  Required: Request cancellation of update. This op
 *			  is called from the context of a different kernel
 *			  thread, so race conditions need to be considered.
 * @cleanup:		  Optional: Complements the prepare()
 *			  function and is called at the completion
 *			  of the update, on success or failure, if the
 *			  prepare function succeeded.
 */
struct fw_upload_ops {
	enum fw_upload_err (*prepare)(struct fw_upload *fw_upload,
				      const u8 *data, u32 size);
	enum fw_upload_err (*write)(struct fw_upload *fw_upload,
				    const u8 *data, u32 offset,
				    u32 size, u32 *written);
	enum fw_upload_err (*poll_complete)(struct fw_upload *fw_upload);
	void (*cancel)(struct fw_upload *fw_upload);
	void (*cleanup)(struct fw_upload *fw_upload);
};

struct fw_upload *
fpga_image_load_register(struct module *module, struct device *parent,
			 const char *name, const struct fw_upload_ops *ops,
			 void *dd_handle);
void fpga_image_load_unregister(struct fw_upload *fw_upload);

#endif
