// SPDX-License-Identifier: GPL-2.0
/*
 * DFL device driver for XYZ private feature
 *
 * Copyright (C) 2023 Intel Corporation, Inc.
 *
 * Authors:
 *   Tim Whisonant <tim.whisonant@intel.com>
 *   Ananda Ravuri <ananda.ravuri@intel.com>
 *   Russell H. Weight <russell.h.weight@intel.com>
 */
#include <linux/bitfield.h>
#include <linux/cdev.h>
#include <linux/dfl.h>
#include <linux/errno.h>
#include <linux/highmem.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/io-64-nonatomic-lo-hi.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#include <linux/xyz-dfl.h>

#define DFL_XYZ_CACHE_SHIFT		6

#define DFL_XYZ_DRIVER_NAME		"dfl-xyz"

#define FME_FEATURE_ID_HE_CACHE		0x25
#define DFL_FPGA_FEATURE_DEV_XYZ	"dfl-xyz"

#define DFL_XYZ_DSM_BASE		0x030
#define DFL_XYZ_WR_ADDR_TABLE_DATA	0x068
#define DFL_XYZ_RD_ADDR_TABLE_DATA	0x088

struct dfl_xyz {
	struct dfl_device *ddev;
	int id;
	struct device *dev;
	struct cdev cdev;
	atomic_t opened;
	void __iomem *mmio_base;
	struct dfl_xyz_region_info rinfo;
	/* Device Status Memory */
	unsigned long dsm_user_addr;
	unsigned long dsm_length;
	struct page **dsm_pages;
	phys_addr_t dsm_phys;
	/* Write Address Table */
	void *wr_addr;
	unsigned long wr_addr_length; /* (in bytes) */
	phys_addr_t wr_addr_phys;
	/* Read Address Table */
	void *rd_addr;
	unsigned long rd_addr_length; /* (in bytes) */
	phys_addr_t rd_addr_phys;
};

static DEFINE_MUTEX(dfl_xyz_class_lock);
static struct class *dfl_xyz_class;
static dev_t dfl_xyz_devt;
static int dfl_xyz_devices;

static inline bool xyz_table_flags_valid(unsigned flags)
{
	if ((flags & DFL_XYZ_READ_ADDR_TABLE) &&
	    (flags & DFL_XYZ_WRITE_ADDR_TABLE))
		return false;
	if (!(flags & GENMASK(1, 0)))
		return false;
	return true;
}

static inline bool xyz_is_read_table(unsigned flags)
{
	return (flags & DFL_XYZ_READ_ADDR_TABLE) != 0;
}

static int dfl_xyz_open(struct inode *inode, struct file *filp)
{
	struct dfl_xyz *xyz = container_of(inode->i_cdev,
					   struct dfl_xyz, cdev);

	if (atomic_cmpxchg(&xyz->opened, 0, 1))
		return -EBUSY;

	filp->private_data = xyz;

        return 0;
}


static long xyz_ioctl_check_extension(struct dfl_xyz *xyz, unsigned long arg)
{
        /* No extension support for now */
        return 0;
}

static long xyz_ioctl_get_region_info(struct dfl_xyz *xyz, void __user *arg)
{
	struct dfl_xyz_region_info rinfo;
	unsigned long minsz;

	minsz = offsetofend(struct dfl_xyz_region_info, offset);

	if (copy_from_user(&rinfo, arg, minsz))
		return -EFAULT;

	if (rinfo.argsz < minsz)
		return -EINVAL;

	rinfo.flags = xyz->rinfo.flags;
	rinfo.size = xyz->rinfo.size;
	rinfo.offset = xyz->rinfo.offset;

	if (copy_to_user(arg, &rinfo, sizeof(rinfo)))
		return -EFAULT;

	return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 3, 0)

static long xyz_adjust_locked_vm(struct device *dev, long npages, bool incr)
{
	unsigned long locked, lock_limit;
	int ret = 0;

	/* the task is exiting. */
	if (!current->mm)
		return 0;

	down_write(&current->mm->mmap_sem);

	if (incr) {
		locked = current->mm->locked_vm + npages;
		lock_limit = rlimit(RLIMIT_MEMLOCK) >> PAGE_SHIFT;

		if (locked > lock_limit && !capable(CAP_IPC_LOCK))
			ret = -ENOMEM;
		else
			current->mm->locked_vm += npages;
	} else {
		if (WARN_ON_ONCE(npages > current->mm->locked_vm))
			npages = current->mm->locked_vm;
		current->mm->locked_vm -= npages;
	}

	dev_dbg(dev, "[%d] RLIMIT_MEMLOCK %c%ld %ld/%ld%s\n", current->pid,
		     incr ? '+' : '-',
		     npages << PAGE_SHIFT,
		     current->mm->locked_vm << PAGE_SHIFT,
		     rlimit(RLIMIT_MEMLOCK),
		     ret ? "- execeeded" : "");

	up_write(&current->mm->mmap_sem);

	return ret;
}

static long xyz_pin_pages(struct device *dev, struct page ***pages,
			  unsigned long user_addr, unsigned long length)
{
	const long npages = length >> PAGE_SHIFT;
	long ret, pinned;

	ret = xyz_adjust_locked_vm(dev, npages, true);
	if (ret)
		return ret;

	*pages = kcalloc(npages, sizeof(struct page *), GFP_KERNEL);
	if (!*pages) {
		xyz_adjust_locked_vm(dev, npages, false);
		return -ENOMEM;
	}

	pinned = get_user_pages_fast(user_addr, npages, 1, *pages);

	if (pinned < 0) {
		ret = pinned;
		goto err_put_pages;
	} else if (pinned != npages) {
		ret = -EFAULT;
		goto err;
	}

	dev_dbg(dev, "%ld pages pinned\n", pinned);

	return 0;

err_put_pages:
	put_all_pages(*pages, pinned);
err:
	kfree(*pages);
	*pages = NULL;
	xyz_adjust_locked_vm(dev, npages, false);
	return ret;
}

static void xyz_unpin_pages(struct device *dev, struct page ***pages,
			    unsigned long length)
{
	const long npages = length >> PAGE_SHIFT;

	if (*pages) {
		put_all_pages(*pages, npages);
		kfree(*pages);
		*pages = NULL;
		xyz_adjust_locked_vm(dev, npages, false);

		dev_dbg(dev, "%ld pages unpinned\n", npages);
	}
}

#else /* < KERNEL_VERSION(5, 3, 0) */

static long xyz_pin_pages(struct device *dev, struct page ***pages,
			  unsigned long user_addr, unsigned long length)
{
	const unsigned int flags = FOLL_LONGTERM | FOLL_WRITE;
	const int npages = PFN_DOWN(length);
	int pinned;
	long ret;

	ret = account_locked_vm(current->mm, npages, true);
	if (ret)
		return ret;

	*pages = kcalloc(npages, sizeof(struct page *), GFP_KERNEL);
	if (!*pages) {
		ret = -ENOMEM;
		goto unlock_vm;
	}

	pinned = pin_user_pages_fast(user_addr, npages, flags, *pages);
	if (pinned < 0) {
		ret = pinned;
		goto free_pages;
	} else if (pinned != npages) {
		ret = -EFAULT;
		goto unpin_pages;
	}

	dev_dbg(dev, "%d pages pinned\n", pinned);

	return 0;

unpin_pages:
	unpin_user_pages(*pages, pinned);
free_pages:
	kfree(*pages);
	*pages = NULL;
unlock_vm:
	account_locked_vm(current->mm, npages, false);
	return ret;
}

static void xyz_unpin_pages(struct device *dev, struct page ***pages,
			    unsigned long length)
{
	const long npages = PFN_DOWN(length);

	if (*pages) {
		unpin_user_pages(*pages, npages);
		kfree(*pages);
		*pages = NULL;

		account_locked_vm(current->mm, npages, false);

		dev_dbg(dev, "%ld pages unpinned\n", npages);
	}
}

#endif /* < KERNEL_VERSION(5, 3, 0) */

/**
 * xyz_dsm_check_continuous_pages - check if pages are continuous
 * @region: dma memory region
 *
 * Return true if pages of given dma memory region have continuous physical
 * address, otherwise return false.
 */
static bool xyz_check_continuous_pages(struct page **pages,
				       unsigned long length)
{
	const int npages = PFN_DOWN(length);
	int i;

	for (i = 0; i < npages - 1; i++)
		if (page_to_pfn(pages[i]) + 1 !=
				page_to_pfn(pages[i + 1]))
			return false;

	return true;
}

static long xyz_set_dsm(struct dfl_xyz *xyz, struct dfl_xyz_dsm_info *dsm_info)
{
	long ret;

	xyz->dsm_user_addr = dsm_info->user_addr;
	xyz->dsm_length = dsm_info->length;

	/*
	 * Check Inputs, only accept page-aligned user memory region with
	 * valid length.
	 */
	if (!PAGE_ALIGNED(xyz->dsm_user_addr) ||
	    !PAGE_ALIGNED(xyz->dsm_length) || !xyz->dsm_length)
		return -EINVAL;

	/* Check overflow */
	if (xyz->dsm_user_addr + xyz->dsm_length < xyz->dsm_user_addr)
		return -EINVAL;

        /* Pin the user memory region */
	ret = xyz_pin_pages(xyz->dev, &xyz->dsm_pages,
			    xyz->dsm_user_addr, xyz->dsm_length);
	if (ret) {
		dev_err(xyz->dev, "failed to pin dsm\n");
		return ret;
	}

	/* Only accept continuous pages, return error else */
	if (!xyz_check_continuous_pages(xyz->dsm_pages,
					xyz->dsm_length)) {
		dev_err(xyz->dev, "pages are not continuous\n");
		ret = -EINVAL;
		goto out_unpin_pages;
	}

	xyz->dsm_phys = page_to_phys(xyz->dsm_pages[0]);

	writeq(xyz->dsm_phys, xyz->mmio_base + DFL_XYZ_DSM_BASE);

	dev_dbg(xyz->dev, "set DSM_BASE: 0x%llx\n", xyz->dsm_phys);

	return 0;

out_unpin_pages:
	xyz_unpin_pages(xyz->dev, &xyz->dsm_pages, xyz->dsm_length);

	return ret;
}

static void xyz_clear_dsm(struct dfl_xyz *xyz)
{
	writeq(0ULL, xyz->mmio_base + DFL_XYZ_DSM_BASE);

	xyz_unpin_pages(xyz->dev, &xyz->dsm_pages, xyz->dsm_length);

	xyz->dsm_user_addr = 0;
	xyz->dsm_length = 0;
	xyz->dsm_phys = 0;
}

static long xyz_ioctl_set_dsm_info(struct dfl_xyz *xyz, void __user *arg)
{
	struct dfl_xyz_dsm_info dsm_info;
	unsigned long minsz;
	long ret;

	minsz = offsetofend(struct dfl_xyz_dsm_info, length);

	if (copy_from_user(&dsm_info, arg, minsz))
		return -EFAULT;

	if (dsm_info.argsz < minsz)
		return -EINVAL;

	ret = xyz_set_dsm(xyz, &dsm_info);
	if (ret) {
		xyz_clear_dsm(xyz);
		dev_err(xyz->dev,
			"xyz_set_dsm failed: %ld\n",
			ret);
	} else
		dev_dbg(xyz->dev,
			"device status mem: ua=0x%lx, len=%lu, phys=0x%llx\n",
			xyz->dsm_user_addr, xyz->dsm_length, xyz->dsm_phys);

	return ret;
}

static void xyz_free_table(struct device *dev, void **addr,
			   unsigned long *length, phys_addr_t *phys,
			   void __iomem *addr_table_data)
{
	if (*addr) {
		kfree(*addr);
		*addr = NULL;
		*length = 0;
		*phys = 0ULL;
		writeq(0ULL, addr_table_data);
	}
}

static long xyz_alloc_table(bool rd, struct dfl_xyz_addr_table *table,
			    struct device *dev, void **addr,
			    unsigned long *length, phys_addr_t *phys,
			    void __iomem *addr_table_data)
{
	unsigned long len;

	if (*addr) {
		dev_err(dev, "xyz_alloc_table: %s addr table already allocated\n",
			rd ? "read" : "write");
		return -EAGAIN;
	}

	if ((table->cache_lines < DFL_XYZ_NUM_LINES_MIN) ||
	    (table->cache_lines > DFL_XYZ_NUM_LINES_MAX)) {
		dev_err(dev, "xyz_alloc_table: invalid num lines: %u\n",
			table->cache_lines);
		return -EINVAL;
	}

	len = table->cache_lines * (1 << DFL_XYZ_CACHE_SHIFT);
	len = ALIGN(len, PAGE_SIZE);

	*addr = kzalloc(len, GFP_KERNEL);
	if (!*addr) {
		dev_err(dev, "kzalloc failed.\n");
		return -ENOMEM;
	}

	*phys = page_to_phys(kmap_to_page(*addr));
	*length = len;

	writeq(*phys, addr_table_data);

	dev_dbg(dev, "set %s_ADDR_TABLE_DATA: 0x%llx\n",
		rd ? "RD" : "WR", *phys);

	return 0;
}

static long xyz_ioctl_alloc_addr_table(struct dfl_xyz *xyz, void __user *arg)
{
	struct dfl_xyz_addr_table table;
	unsigned long minsz;
	long ret;
	bool rd;

	minsz = offsetofend(struct dfl_xyz_addr_table, cache_lines);

	if (copy_from_user(&table, arg, minsz))
		return -EFAULT;

	if (table.argsz < minsz)
		return -EINVAL;

	if (!xyz_table_flags_valid(table.flags))
		return -EINVAL;

	rd = xyz_is_read_table(table.flags);

	ret = xyz_alloc_table(rd, &table, xyz->dev,
		rd ? &xyz->rd_addr : &xyz->wr_addr,
		rd ? &xyz->rd_addr_length : &xyz->wr_addr_length,
		rd ? &xyz->rd_addr_phys : &xyz->wr_addr_phys,
		xyz->mmio_base + (rd ? DFL_XYZ_RD_ADDR_TABLE_DATA :
				       DFL_XYZ_WR_ADDR_TABLE_DATA));
	if (ret) {
		xyz_free_table(xyz->dev,
			rd ? &xyz->rd_addr : &xyz->wr_addr,
			rd ? &xyz->rd_addr_length : &xyz->wr_addr_length,
			rd ? &xyz->rd_addr_phys : &xyz->wr_addr_phys,
			xyz->mmio_base + (rd ? DFL_XYZ_RD_ADDR_TABLE_DATA :
					       DFL_XYZ_WR_ADDR_TABLE_DATA));
		dev_err(xyz->dev,
			"xyz_alloc_table failed: %ld\n", ret);
	}

	return ret;
}

static long xyz_ioctl_free_addr_table(struct dfl_xyz *xyz, void __user *arg)
{
	struct dfl_xyz_addr_table table;
	unsigned long minsz;
	bool rd;

	minsz = offsetofend(struct dfl_xyz_addr_table, cache_lines);

	if (copy_from_user(&table, arg, minsz))
		return -EFAULT;

	if (table.argsz < minsz)
		return -EINVAL;

	if (!xyz_table_flags_valid(table.flags))
		return -EINVAL;

	if (table.cache_lines) /* unused for free */
		return -EINVAL;

	rd = xyz_is_read_table(table.flags);

	xyz_free_table(xyz->dev,
		rd ? &xyz->rd_addr : &xyz->wr_addr,
		rd ? &xyz->rd_addr_length : &xyz->wr_addr_length,
		rd ? &xyz->rd_addr_phys : &xyz->wr_addr_phys,
		xyz->mmio_base + (rd ? DFL_XYZ_RD_ADDR_TABLE_DATA :
				       DFL_XYZ_WR_ADDR_TABLE_DATA));

	return 0;
}

static long xyz_append_table(bool rd, struct dfl_xyz_addr_table *table,
			     struct device *dev, void *addr,
			     unsigned long length, phys_addr_t phys,
			     void __iomem *addr_table_data)
{
	const unsigned long cl = (1 << DFL_XYZ_CACHE_SHIFT);
	unsigned long len;

	if (!addr) {
		dev_err(dev, "%s table is not allocated.\n",
			rd ? "read" : "write");
		return -EFAULT;
	}

	len = table->cache_lines * cl;

	if ((len + cl) > length) {
		dev_err(dev,
			"%s offset out of bounds: %lu (max %lu).\n",
			rd ? "read" : "write", len, length - cl);
		return -EFAULT;
	}

	writeq(phys + len, addr_table_data);

	return 0;
}

static long xyz_ioctl_append_addr_table(struct dfl_xyz *xyz, void __user *arg)
{
	struct dfl_xyz_addr_table table;
	unsigned long minsz;
	bool rd;

	minsz = offsetofend(struct dfl_xyz_addr_table, cache_lines);

	if (copy_from_user(&table, arg, minsz))
		return -EFAULT;

	if (table.argsz < minsz)
		return -EINVAL;

	if (!xyz_table_flags_valid(table.flags))
		return -EINVAL;

	rd = xyz_is_read_table(table.flags);

	return xyz_append_table(rd, &table, xyz->dev,
		rd ? xyz->rd_addr : xyz->wr_addr,
		rd ? xyz->rd_addr_length : xyz->wr_addr_length,
		rd ? xyz->rd_addr_phys : xyz->wr_addr_phys,
		xyz->mmio_base + (rd ? DFL_XYZ_RD_ADDR_TABLE_DATA :
				       DFL_XYZ_WR_ADDR_TABLE_DATA));
}

static long dfl_xyz_ioctl(struct file *filp, unsigned int cmd,
			  unsigned long arg)
{
	struct dfl_xyz *xyz = filp->private_data;

        switch (cmd) {
	case DFL_XYZ_GET_API_VERSION:
		return DFL_XYZ_API_VERSION;
	case DFL_XYZ_CHECK_EXTENSION:
		return xyz_ioctl_check_extension(xyz, arg);
	case DFL_XYZ_GET_REGION_INFO:
		return xyz_ioctl_get_region_info(xyz, (void __user *)arg);
	case DFL_XYZ_SET_DSM_INFO:
		return xyz_ioctl_set_dsm_info(xyz, (void __user *)arg);
	case DFL_XYZ_CLEAR_DSM_INFO:
		xyz_clear_dsm(xyz);
		return 0;
	case DFL_XYZ_ALLOC_ADDR_TABLE:
		return xyz_ioctl_alloc_addr_table(xyz, (void __user *)arg);
	case DFL_XYZ_FREE_ADDR_TABLE:
		return xyz_ioctl_free_addr_table(xyz, (void __user *)arg);
	case DFL_XYZ_APPEND_ADDR_TABLE:
		return xyz_ioctl_append_addr_table(xyz, (void __user *)arg);
        }

        return -EINVAL;
}

static const struct vm_operations_struct xyz_vma_ops = {
#ifdef CONFIG_HAVE_IOREMAP_PROT
	.access = generic_access_phys,
#endif
};

static int dfl_xyz_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct dfl_xyz *xyz = filp->private_data;
	u64 size = vma->vm_end - vma->vm_start;
	u64 offset;

	if (!(vma->vm_flags & VM_SHARED))
		return -EINVAL;

	if (!(xyz->rinfo.flags & DFL_XYZ_REGION_MMAP))
		return -EINVAL;

	if ((vma->vm_flags & VM_READ) &&
	    !(xyz->rinfo.flags & DFL_XYZ_REGION_READ))
		return -EPERM;

	if ((vma->vm_flags & VM_WRITE) &&
	    !(xyz->rinfo.flags & DFL_XYZ_REGION_WRITE))
                return -EPERM;

	offset = PFN_PHYS(vma->vm_pgoff);

	/* Support debug access to the mapping */
	vma->vm_ops = &xyz_vma_ops;

	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	return remap_pfn_range(vma, vma->vm_start,
			       PFN_DOWN(xyz->ddev->mmio_res.start + (offset - xyz->rinfo.offset)),
			       size, vma->vm_page_prot);
}

static int dfl_xyz_release(struct inode *inode, struct file *filp)
{
	struct dfl_xyz *xyz = filp->private_data;

	xyz_clear_dsm(xyz);
	xyz_free_table(xyz->dev, &xyz->wr_addr, &xyz->wr_addr_length,
		       &xyz->wr_addr_phys,
		       xyz->mmio_base + DFL_XYZ_WR_ADDR_TABLE_DATA);
	xyz_free_table(xyz->dev, &xyz->rd_addr, &xyz->rd_addr_length,
		       &xyz->rd_addr_phys,
		       xyz->mmio_base + DFL_XYZ_RD_ADDR_TABLE_DATA);
	atomic_set(&xyz->opened, 0);

	return 0;
}

static const struct file_operations dfl_xyz_fops = {
	.owner = THIS_MODULE,
	.open = dfl_xyz_open,
	.release = dfl_xyz_release,
	.unlocked_ioctl = dfl_xyz_ioctl,
	.mmap = dfl_xyz_mmap,
};

static void xyz_dev_release(struct device *dev)
{
	struct dfl_xyz *xyz = dev_get_drvdata(dev);

	cdev_del(&xyz->cdev);
}

static void xyz_chardev_uinit(struct dfl_xyz *xyz)
{
	dev_set_drvdata(&xyz->ddev->dev, NULL);
	device_destroy(dfl_xyz_class,
		       MKDEV(MAJOR(dfl_xyz_devt), xyz->id));
}

static int xyz_chardev_init(struct dfl_xyz *xyz,
			    struct dfl_device *ddev,
			    void __iomem *mmio_base)
{
	int ret;

	dev_set_drvdata(&ddev->dev, xyz);
	xyz->ddev = ddev;
	xyz->mmio_base = mmio_base;
	xyz->id = dfl_xyz_devices++;

	xyz->rinfo.argsz = sizeof(struct dfl_xyz_region_info);
	xyz->rinfo.flags = DFL_XYZ_REGION_READ | DFL_XYZ_REGION_WRITE |
			   DFL_XYZ_REGION_MMAP;
	xyz->rinfo.size = resource_size(&ddev->mmio_res);
	xyz->rinfo.offset = 0;

	xyz->dev = device_create(dfl_xyz_class, &ddev->dev,
				 MKDEV(MAJOR(dfl_xyz_devt), xyz->id),
				 xyz, DFL_FPGA_FEATURE_DEV_XYZ ".%d",
				 xyz->id);
	if (IS_ERR(xyz->dev)) {
		ret = PTR_ERR(xyz->dev);
		dev_err(&ddev->dev,
			"device_create failed: %d\n",
			ret);
		xyz->dev = NULL;
		return ret;
	}
	xyz->dev->release = xyz_dev_release;

	dev_dbg(xyz->dev, "added xyz device: %s\n", dev_name(xyz->dev));

	cdev_init(&xyz->cdev, &dfl_xyz_fops);
	xyz->cdev.owner = THIS_MODULE;
	xyz->cdev.ops = &dfl_xyz_fops;

	ret = cdev_add(&xyz->cdev, xyz->dev->devt, 1);
	if (ret)
		dev_err(xyz->dev,
			"cdev_add failed: %d\n", ret);

	return ret;
}

static int dfl_xyz_probe(struct dfl_device *ddev)
{
	void __iomem *mmio_base;
	struct dfl_xyz *xyz;
	bool unlock = true;
	int ret = 0;

	dev_dbg(&ddev->dev, "entering dfl_xyz_probe\n");

	if (unlikely(mutex_lock_interruptible(&dfl_xyz_class_lock))) {
		dev_err(&ddev->dev,
			"mutex_lock_interruptible failed.\n");
		unlock = false;
	}

	if (!dfl_xyz_class) {
		dfl_xyz_class = class_create(THIS_MODULE,
					     DFL_XYZ_DRIVER_NAME);
		if (IS_ERR(dfl_xyz_class)) {
			ret = PTR_ERR(dfl_xyz_class);
			dfl_xyz_class = NULL;
			dev_err(&ddev->dev,
				"class_create failed: %d\n", ret);
			goto out_unlock;
		}
	}

	if (!MAJOR(dfl_xyz_devt)) {
		ret = alloc_chrdev_region(&dfl_xyz_devt, 0,
					  MINORMASK,
					  DFL_XYZ_DRIVER_NAME);
		if (ret) {
			dev_err(&ddev->dev,
				"alloc_chrdev_region failed: %d\n",
				ret);
			dfl_xyz_devt = MKDEV(0, 0);
			goto out_unlock;
		}
	
	}

	mmio_base = devm_ioremap_resource(&ddev->dev, &ddev->mmio_res);
	if (IS_ERR(mmio_base)) {
		ret = PTR_ERR(mmio_base);
		dev_err(&ddev->dev,
			"devm_ioremap_resource failed: %d\n", ret);
		goto out_unlock;
	}

	xyz = devm_kzalloc(&ddev->dev, sizeof(*xyz), GFP_KERNEL);
	if (!xyz) {
		dev_err(&ddev->dev,
			"devm_kzalloc failed.\n");
		ret = -ENOMEM;
		goto out_unlock;
	}

	ret = xyz_chardev_init(xyz, ddev, mmio_base);
	if (ret)
		dev_err(&ddev->dev,
			"xyz_chardev_init failed: %d\n", ret);

out_unlock:
	if (unlock)
		mutex_unlock(&dfl_xyz_class_lock);

	return ret;
}

static void dfl_xyz_remove(struct dfl_device *ddev)
{
	struct dfl_xyz *xyz = dev_get_drvdata(&ddev->dev);
	bool unlock = true;

	xyz_chardev_uinit(xyz);

	if (unlikely(mutex_lock_interruptible(&dfl_xyz_class_lock))) {
		dev_err(&ddev->dev,
			"mutex_lock_interruptible failed.\n");
		unlock = false;
	}

	if (--dfl_xyz_devices <= 0) {
		if (dfl_xyz_class) {
			class_destroy(dfl_xyz_class);
			dfl_xyz_class = NULL;
		}

		if (MAJOR(dfl_xyz_devt)) {
			unregister_chrdev_region(dfl_xyz_devt, MINORMASK);
			dfl_xyz_devt = MKDEV(0, 0);
		}
	}

	if (unlock)
		mutex_unlock(&dfl_xyz_class_lock);
}

static const struct dfl_device_id dfl_xyz_ids[] = {
	{ FME_ID, FME_FEATURE_ID_HE_CACHE },
	{ }
};
MODULE_ALIAS("dfl:t0000f0025");
MODULE_DEVICE_TABLE(dfl, dfl_xyz_ids);

static struct dfl_driver dfl_xyz_driver = {
	.drv	= {
		.name	= DFL_XYZ_DRIVER_NAME,
	},
	.id_table = dfl_xyz_ids,
	.probe   = dfl_xyz_probe,
	.remove = dfl_xyz_remove,
};
module_dfl_driver(dfl_xyz_driver);

MODULE_DESCRIPTION("DFL XYZ driver");
MODULE_AUTHOR("Intel Corporation");
MODULE_LICENSE("GPL v2");
