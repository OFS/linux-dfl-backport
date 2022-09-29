// SPDX-License-Identifier: GPL-2.0
/*
 * FPGA Image Load Framework
 *
 * Copyright (C) 2019-2021 Intel Corporation, Inc.
 */

#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/fpga/fpga-image-load.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>

#define IMAGE_LOAD_XA_LIMIT	XA_LIMIT(0, INT_MAX)
static DEFINE_XARRAY_ALLOC(fpga_image_load_xa);

static struct class *fpga_image_load_class;
static dev_t fpga_image_devt;

#define to_image_load(d) container_of(d, struct fpga_image_load, dev)

/**
 * enum fw_upload_prog - firmware upload progress codes
 * @FW_UPLOAD_PROG_IDLE: there is no firmware upload in progress
 * @FW_UPLOAD_PROG_RECEIVING: worker thread is receiving firmware data
 * @FW_UPLOAD_PROG_PREPARING: target device is preparing for firmware upload
 * @FW_UPLOAD_PROG_TRANSFERRING: data is being copied to the device
 * @FW_UPLOAD_PROG_PROGRAMMING: device is performing the firmware update
 * @FW_UPLOAD_PROG_MAX: Maximum progress code marker
 */
enum fw_upload_prog {
	FW_UPLOAD_PROG_IDLE,
	FW_UPLOAD_PROG_RECEIVING,
	FW_UPLOAD_PROG_PREPARING,
	FW_UPLOAD_PROG_TRANSFERRING,
	FW_UPLOAD_PROG_PROGRAMMING,
	FW_UPLOAD_PROG_MAX
};

struct fw_upload_priv {
	struct fw_upload *fw_upload;
	struct module *module;
	const char *name;
	const struct fw_upload_ops *ops;
	struct mutex lock;		  /* protect data structure contents */
	struct work_struct work;
	const u8 *data;			  /* pointer to update data */
	u32 remaining_size;		  /* size remaining to transfer */
	enum fw_upload_prog progress;
	enum fw_upload_prog err_progress; /* progress at time of failure */
	enum fw_upload_err err_code;	  /* security manager error code */
	struct eventfd_ctx *finished;
};

struct fpga_image_load {
	struct device dev;
	struct cdev cdev;
	atomic_t opened;
	void *fw_upload_priv;
};

static void fw_upload_update_progress(struct fw_upload_priv *fwlp,
				      enum fw_upload_prog new_progress)
{
	mutex_lock(&fwlp->lock);
	fwlp->progress = new_progress;
	mutex_unlock(&fwlp->lock);
}

static void fw_upload_set_error(struct fw_upload_priv *fwlp,
				enum fw_upload_err err_code)
{
	mutex_lock(&fwlp->lock);
	fwlp->err_progress = fwlp->progress;
	fwlp->err_code = err_code;
	mutex_unlock(&fwlp->lock);
}

static void fw_upload_prog_complete(struct fw_upload_priv *fwlp)
{
	mutex_lock(&fwlp->lock);
	fwlp->progress = FW_UPLOAD_PROG_IDLE;
	eventfd_signal(fwlp->finished, 1);
	mutex_unlock(&fwlp->lock);
}

static void fw_upload_main(struct work_struct *work)
{
	struct fpga_image_load *imgld;
	struct fw_upload_priv *fwlp;
	u32 written = 0, offset = 0;
	enum fw_upload_err ret;
	struct device *dev;
	struct fw_upload *fwl;

	fwlp = container_of(work, struct fw_upload_priv, work);
	fwl = fwlp->fw_upload;
	imgld = (struct fpga_image_load *)fwl->priv;
	dev = &imgld->dev;

	fw_upload_update_progress(fwlp, FW_UPLOAD_PROG_PREPARING);
	ret = fwlp->ops->prepare(fwl, fwlp->data, fwlp->remaining_size);
	if (ret != FW_UPLOAD_ERR_NONE) {
		fw_upload_set_error(fwlp, ret);
		goto putdev_exit;
	}

	fw_upload_update_progress(fwlp, FW_UPLOAD_PROG_TRANSFERRING);
	while (fwlp->remaining_size) {
		ret = fwlp->ops->write(fwl, fwlp->data, offset,
					fwlp->remaining_size, &written);
		if (ret != FW_UPLOAD_ERR_NONE || !written) {
			if (ret == FW_UPLOAD_ERR_NONE) {
				dev_warn(dev, "write-op wrote zero data\n");
				ret = FW_UPLOAD_ERR_RW_ERROR;
			}
			fw_upload_set_error(fwlp, ret);
			goto done;
		}

		fwlp->remaining_size -= written;
		offset += written;
	}

	fw_upload_update_progress(fwlp, FW_UPLOAD_PROG_PROGRAMMING);
	ret = fwlp->ops->poll_complete(fwl);
	if (ret != FW_UPLOAD_ERR_NONE)
		fw_upload_set_error(fwlp, ret);

done:
	if (fwlp->ops->cleanup)
		fwlp->ops->cleanup(fwl);

putdev_exit:
	put_device(dev->parent);

	/*
	 * Note: fwlp->remaining_size is left unmodified here to provide
	 * additional information on errors. It will be reinitialized when
	 * the next firmware upload begins.
	 */
	vfree(fwlp->data);
	fwlp->data = NULL;
	fw_upload_prog_complete(fwlp);
	eventfd_ctx_put(fwlp->finished);
	fwlp->finished = NULL;
}

static int fpga_image_load_ioctl_write(struct fw_upload_priv *fwlp,
				       unsigned long arg)
{
	struct fpga_image_load *imgld;
	struct fpga_image_write wb;
	struct fw_upload *fwl;
	unsigned long minsz;
	struct device *dev;
	int ret;
	u8 *buf;

	fwl = fwlp->fw_upload;
	imgld = (struct fpga_image_load *)fwl->priv;
	dev = &imgld->dev;

	minsz = offsetofend(struct fpga_image_write, buf);
	if (copy_from_user(&wb, (void __user *)arg, minsz))
		return -EFAULT;

	if (wb.flags)
		return -EINVAL;

	if (!wb.size)
		return -EINVAL;

	if (wb.evtfd < 0)
		return -EINVAL;

	buf = vzalloc(wb.size);
	if (!buf)
		return -ENOMEM;

	if (copy_from_user(buf, u64_to_user_ptr(wb.buf), wb.size)) {
		ret = -EFAULT;
		goto exit_free;
	}

	fwlp->finished = eventfd_ctx_fdget(wb.evtfd);
	if (IS_ERR(fwlp->finished)) {
		ret = PTR_ERR(fwlp->finished);
		fwlp->finished = NULL;
		goto exit_free;
	}

	get_device(dev->parent); /* released in fw_upload_main */

	fwlp->progress = FW_UPLOAD_PROG_RECEIVING;
	fwlp->err_code = 0;
	fwlp->remaining_size = wb.size;
	fwlp->data = buf;

	queue_work(system_long_wq, &fwlp->work);
	return 0;

exit_free:
	vfree(buf);
	return ret;
}

static const int fw_upload_prog_code[] = {
	[FW_UPLOAD_PROG_IDLE]	      = FPGA_IMAGE_PROG_IDLE,
	[FW_UPLOAD_PROG_RECEIVING]    = FPGA_IMAGE_PROG_STARTING,
	[FW_UPLOAD_PROG_PREPARING]    = FPGA_IMAGE_PROG_PREPARING,
	[FW_UPLOAD_PROG_TRANSFERRING] = FPGA_IMAGE_PROG_WRITING,
	[FW_UPLOAD_PROG_PROGRAMMING]  = FPGA_IMAGE_PROG_PROGRAMMING,
	[FW_UPLOAD_PROG_MAX]	      = FPGA_IMAGE_PROG_MAX
};

static const int fw_upload_err_code[] = {
	[FW_UPLOAD_ERR_NONE]	     = 0,
	[FW_UPLOAD_ERR_HW_ERROR]     = FPGA_IMAGE_ERR_HW_ERROR,
	[FW_UPLOAD_ERR_TIMEOUT]	     = FPGA_IMAGE_ERR_TIMEOUT,
	[FW_UPLOAD_ERR_CANCELED]     = FPGA_IMAGE_ERR_CANCELED,
	[FW_UPLOAD_ERR_BUSY]	     = FPGA_IMAGE_ERR_BUSY,
	[FW_UPLOAD_ERR_INVALID_SIZE] = FPGA_IMAGE_ERR_INVALID_SIZE,
	[FW_UPLOAD_ERR_RW_ERROR]     = FPGA_IMAGE_ERR_RW_ERROR,
	[FW_UPLOAD_ERR_WEAROUT]	     = FPGA_IMAGE_ERR_WEAROUT,
	[FW_UPLOAD_ERR_MAX]	     = FPGA_IMAGE_ERR_MAX
};

static const int fw_upload_progress(struct device *dev, enum fw_upload_prog prog)
{
	int status = FPGA_IMAGE_PROG_MAX;

	if (prog < FW_UPLOAD_PROG_MAX)
		status = fw_upload_prog_code[prog];
	else
		dev_err(dev, "Invalid status during secure update: %d\n", prog);

	return status;
}

static const int fw_upload_error(struct device *dev, enum fw_upload_err err_code)
{
	int error = FPGA_IMAGE_ERR_MAX;

	if (err_code < FW_UPLOAD_ERR_MAX)
		error = fw_upload_err_code[err_code];
	else
		dev_err(dev, "Invalid error code during secure update: %d\n",
			err_code);

	return error;
}

static int fpga_image_load_ioctl_status(struct fw_upload_priv *fwlp,
					unsigned long arg)
{
	struct fpga_image_status status;
	struct fpga_image_load *imgld;
	struct fw_upload *fwl;
	struct device *dev;

	fwl = fwlp->fw_upload;
	imgld = (struct fpga_image_load *)fwl->priv;
	dev = &imgld->dev;

	memset(&status, 0, sizeof(status));
	status.progress = fw_upload_progress(dev, fwlp->progress);
	status.remaining_size = fwlp->remaining_size;
	status.err_progress = fw_upload_progress(dev, fwlp->err_progress);
	status.err_code = fw_upload_error(dev, fwlp->err_code);

	if (copy_to_user((void __user *)arg, &status, sizeof(status)))
		return -EFAULT;

	return 0;
}

static int fpga_image_load_ioctl_cancel(struct fw_upload_priv *fwlp,
					unsigned long arg)
{
	if (fwlp->progress == FW_UPLOAD_PROG_IDLE)
		return -ENODEV;

	fwlp->ops->cancel(fwlp->fw_upload);
	return 0;
}

static long fpga_image_load_ioctl(struct file *filp, unsigned int cmd,
				  unsigned long arg)
{
	struct fpga_image_load *imgld = filp->private_data;
	struct fw_upload_priv *fwlp;
	int ret = 0;

	fwlp = imgld->fw_upload_priv;

	mutex_lock(&fwlp->lock);

	switch (cmd) {
	case FPGA_IMAGE_LOAD_WRITE:
		ret = fpga_image_load_ioctl_write(fwlp, arg);
		break;
	case FPGA_IMAGE_LOAD_STATUS:
		ret = fpga_image_load_ioctl_status(fwlp, arg);
		break;
	case FPGA_IMAGE_LOAD_CANCEL:
		ret = fpga_image_load_ioctl_cancel(fwlp, arg);
		break;
	default:
		ret = -ENOTTY;
		break;
	}

	mutex_unlock(&fwlp->lock);

	return ret;
}

static int fpga_image_load_open(struct inode *inode, struct file *filp)
{
	struct fpga_image_load *imgld = container_of(inode->i_cdev,
						     struct fpga_image_load, cdev);

	if (atomic_cmpxchg(&imgld->opened, 0, 1))
		return -EBUSY;

	filp->private_data = imgld;

	return 0;
}

static int fpga_image_load_release(struct inode *inode, struct file *filp)
{
	struct fpga_image_load *imgld = filp->private_data;
	struct fw_upload_priv *fwlp;

	fwlp = imgld->fw_upload_priv;

	mutex_lock(&fwlp->lock);
	if (fwlp->progress == FW_UPLOAD_PROG_IDLE) {
		mutex_unlock(&fwlp->lock);
		goto close_exit;
	}

	fwlp->ops->cancel(fwlp->fw_upload);

	mutex_unlock(&fwlp->lock);
	flush_work(&fwlp->work);

close_exit:
	atomic_set(&imgld->opened, 0);

	return 0;
}

static const struct file_operations fpga_image_load_fops = {
	.owner = THIS_MODULE,
	.open = fpga_image_load_open,
	.release = fpga_image_load_release,
	.unlocked_ioctl = fpga_image_load_ioctl,
};

/**
 * fpga_image_load_register - create and register an FPGA Image Load Device
 *
 * @module: kernel module of this device
 * @parent: fpga image load device from pdev
 * @name: firmware name - unused by image load driver
 * @ops:   pointer to a structure of image load callback functions
 * @dd_handle: pointer to the lower-level device driver
 *
 * Return: struct fw_upload pointer or ERR_PTR()
 */
struct fw_upload *
fpga_image_load_register(struct module *module, struct device *parent,
			 const char *name, const struct fw_upload_ops *ops,
			 void *dd_handle)
{
	struct fw_upload_priv *fw_upload_priv;
	struct fw_upload *fw_upload;
	struct fpga_image_load *imgld;
	int ret;

	if (!ops || !ops->cancel || !ops->prepare ||
	    !ops->write || !ops->poll_complete) {
		dev_err(parent, "Attempt to register without all required ops\n");
		return ERR_PTR(-ENOMEM);
	}

	if (!try_module_get(module))
		return ERR_PTR(-EFAULT);

	fw_upload = kzalloc(sizeof(*fw_upload), GFP_KERNEL);
	if (!fw_upload) {
		ret = -ENOMEM;
		goto exit_module_put;
	}

	fw_upload_priv = kzalloc(sizeof(*fw_upload_priv), GFP_KERNEL);
	if (!fw_upload_priv) {
		ret = -ENOMEM;
		goto free_fw_upload;
	}

	fw_upload_priv->fw_upload = fw_upload;
	fw_upload_priv->ops = ops;
	mutex_init(&fw_upload_priv->lock);
	fw_upload_priv->module = module;
	fw_upload_priv->name = name;
	fw_upload_priv->err_code = 0;
	fw_upload_priv->progress = FW_UPLOAD_PROG_IDLE;
	fw_upload_priv->finished = NULL;
	INIT_WORK(&fw_upload_priv->work, fw_upload_main);
	fw_upload->dd_handle = dd_handle;

	imgld = kzalloc(sizeof(*imgld), GFP_KERNEL);
	if (!imgld) {
		ret = -ENOMEM;
		goto free_fw_upload_priv;
	}

	fw_upload->priv = imgld;
	imgld->fw_upload_priv = fw_upload_priv;
	imgld->dev.parent = parent;
	imgld->dev.class = fpga_image_load_class;

	ret = xa_alloc(&fpga_image_load_xa, &imgld->dev.id, imgld,
		       IMAGE_LOAD_XA_LIMIT, GFP_KERNEL);
	if (ret)
		goto free_imgld;
	imgld->dev.devt = MKDEV(MAJOR(fpga_image_devt), imgld->dev.id);

	ret = dev_set_name(&imgld->dev, "fpga_image_load%d", imgld->dev.id);
	if (ret) {
		dev_err(parent, "Failed to set device name: fpga_image_load%d\n",
			imgld->dev.id);
		goto error_device;
	}

	ret = device_register(&imgld->dev);
	if (ret) {
		put_device(&imgld->dev);
		goto exit_module_put;
	}

	cdev_init(&imgld->cdev, &fpga_image_load_fops);
	imgld->cdev.owner = parent->driver->owner;
	cdev_set_parent(&imgld->cdev, &imgld->dev.kobj);

	ret = cdev_add(&imgld->cdev, imgld->dev.devt, 1);
	if (ret) {
		put_device(&imgld->dev);
		goto exit_module_put;
	}

	return fw_upload;

error_device:
	xa_erase(&fpga_image_load_xa, imgld->dev.id);

free_imgld:
	kfree(imgld);

free_fw_upload_priv:
	kfree(fw_upload_priv);

free_fw_upload:
	kfree(fw_upload);

exit_module_put:
	module_put(module);

	return ERR_PTR(ret);
}
EXPORT_SYMBOL_GPL(fpga_image_load_register);

/**
 * fpga_image_load_unregister - unregister an FPGA image load device
 *
 * @fw_upload: pointer to struct fw_upload
 *
 * This function is intended for use in the parent driver's remove()
 * function.
 */
void fpga_image_load_unregister(struct fw_upload *fw_upload)
{
	struct fpga_image_load *imgld = fw_upload->priv;
	struct fw_upload_priv *fw_upload_priv;
	struct module *module;

	fw_upload_priv = imgld->fw_upload_priv;
	module = fw_upload_priv->module;

	mutex_lock(&fw_upload_priv->lock);
	if (fw_upload_priv->progress == FW_UPLOAD_PROG_IDLE) {
		mutex_unlock(&fw_upload_priv->lock);
		goto unregister;
	}

	fw_upload_priv->ops->cancel(fw_upload);
	mutex_unlock(&fw_upload_priv->lock);

	/* Ensure lower-level device-driver is finished */
	flush_work(&fw_upload_priv->work);

unregister:
	cdev_del(&imgld->cdev);
	device_unregister(&imgld->dev);
	module_put(module);
}
EXPORT_SYMBOL_GPL(fpga_image_load_unregister);

static void fpga_image_load_dev_release(struct device *dev)
{
	struct fpga_image_load *imgld = to_image_load(dev);
	struct fw_upload_priv *fwlp;

	fwlp = imgld->fw_upload_priv;

	xa_erase(&fpga_image_load_xa, imgld->dev.id);
	kfree(fwlp->fw_upload);
	kfree(fwlp);
	kfree(imgld);
}

static int __init fpga_image_load_class_init(void)
{
	int ret;
	pr_info("FPGA Image Load Framework\n");

	fpga_image_load_class = class_create(THIS_MODULE, "fpga_image_load");
	if (IS_ERR(fpga_image_load_class))
		return PTR_ERR(fpga_image_load_class);

	ret = alloc_chrdev_region(&fpga_image_devt, 0, MINORMASK,
				  "fpga_image_load");
	if (ret)
		goto exit_destroy_class;

	fpga_image_load_class->dev_release = fpga_image_load_dev_release;

	return 0;

exit_destroy_class:
	class_destroy(fpga_image_load_class);
	return ret;
}

static void __exit fpga_image_load_class_exit(void)
{
	unregister_chrdev_region(fpga_image_devt, MINORMASK);
	class_destroy(fpga_image_load_class);
	WARN_ON(!xa_empty(&fpga_image_load_xa));
}

MODULE_DESCRIPTION("FPGA Image Load Framework");
MODULE_LICENSE("GPL v2");

subsys_initcall(fpga_image_load_class_init);
module_exit(fpga_image_load_class_exit)
