/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2024 Intel Corporation
 *
 * This file contains code shared between dfl-afu-dma-region.c and dfl-cxl-cache.c
 */

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 3, 0) && RHEL_RELEASE_CODE < 0x803
static long afu_dma_adjust_locked_vm(struct device *dev, long npages, bool incr)
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
#endif
